#include "miner.h"
#include "proposer.h"
#include "proposal_scope.h"
#include "peer/peer_session_manager.h"

#include "pool.h"
#include "runtime.h"
#include "tasks/task_process_transaction.h"

namespace token{
  Proposer::Proposer(Runtime* runtime):
    runtime_(runtime),
    on_start_(),
    on_phase1_(),
    on_phase2_(),
    on_accepted_(),
    on_rejected_(),
    on_timeout_(){

    on_start_.data = this;
    CHECK_UVRESULT2(FATAL, uv_async_init(runtime->loop(), &on_start_, &HandleOnStart), "cannot initialize the on_start_ callback");
    on_phase1_.data = this;
    CHECK_UVRESULT2(FATAL, uv_async_init(runtime->loop(), &on_phase1_, &HandleOnPhase1), "cannot initialize the on_phase1_ callback");
    on_phase2_.data = this;
    CHECK_UVRESULT2(FATAL, uv_async_init(runtime->loop(), &on_phase2_, &HandleOnPhase2), "cannot initialize the on_phase2_ callback");
    on_accepted_.data = this;
    CHECK_UVRESULT2(FATAL, uv_async_init(runtime->loop(), &on_accepted_, &HandleOnAccepted), "cannot initialize the on_accepted_ callback");
    on_rejected_.data = this;
    CHECK_UVRESULT2(FATAL, uv_async_init(runtime->loop(), &on_rejected_, &HandleOnRejected), "cannot initialize the on_rejected_ callback");
    on_timeout_.data = this;
    CHECK_UVRESULT2(FATAL, uv_async_init(runtime->loop(), &on_timeout_, &HandleOnTimeout), "cannot initialize the on_timeout_ callback");
  }

  class ProposalPhaser{
  public:
    static const uint64_t kTimeoutMilliseconds = 1000 * 60 * 60;
  protected:
    Proposer* proposer_;
    uv_timer_t timeout_;
    atomic::RelaxedAtomic<bool> timedout_;

    inline Proposer*
    proposer() const{
      return proposer_;
    }

    inline void
    Timeout(){
      timedout_ = true;
    }

    inline void
    WaitForRequiredVotes(){
      while(!(TimedOut() || ProposalScope::HasRequiredVotes()));//spin
    }

    inline bool
    FailProposal(){
      NOT_IMPLEMENTED(ERROR);//TODO: implement
      return false;
    }

    inline bool
    TimeoutProposal(){
      return proposer()->OnTimeout();
    }

    inline bool
    AcceptProposal(){
      return proposer()->OnAccepted();
    }

    inline bool
    RejectProposal(){
      return proposer()->OnRejected();
    }

    static void
    OnTimeout(uv_timer_t* handle){
      auto phaser = (ProposalPhaser*)handle->data;
      phaser->Timeout();
    }
  public:
    explicit ProposalPhaser(Proposer* proposer):
      proposer_(proposer),
      timeout_(),
      timedout_(false){
      timeout_.data = this;
      CHECK_UVRESULT2(FATAL, uv_timer_init(proposer->GetRuntime()->loop(), &timeout_), "cannot initialize timeout timer");
    }
    virtual ~ProposalPhaser() = default;

    bool TimedOut() const{
      return (bool)timedout_;
    }

    bool StartTimeoutTimer(){
      VERIFY_UVRESULT2(FATAL, uv_timer_start(&timeout_, &OnTimeout, kTimeoutMilliseconds, 0), "cannot start timeout timer");
      return true;
    }

    bool StopTimeoutTimer(){
      VERIFY_UVRESULT2(FATAL, uv_timer_stop(&timeout_), "cannot stop timeout timer");
      return true;
    }

    virtual bool ExecutePhase(Proposal* proposal) = 0;
  };

  class Phase1 : public ProposalPhaser{
  public:
    explicit Phase1(Proposer* proposer):
      ProposalPhaser(proposer){}
    ~Phase1() override = default;

    bool ExecutePhase(Proposal* proposal) override{
      if(!StartTimeoutTimer()){
        DLOG(WARNING) << "stopping phase1, cannot start timeout timer.";
        return false;
      }

      //TODO:
      // - state check
      ProposalScope::SetPhase(ProposalPhase::kPhase1);
      PeerSessionManager::BroadcastPrepare();
      ProposalScope::Promise(UUID());//TODO: get node_id
      WaitForRequiredVotes();

      if(TimedOut()){
        return TimeoutProposal();
      } else if(!ProposalScope::HasRequiredVotes()){
        return RejectProposal();
      }

      DLOG(INFO) << "phase1 has finished.";
      return true;
    }

    static inline bool
    Execute(Proposer* proposer, Proposal* proposal){
      Phase1 phase(proposer);
      return phase.ExecutePhase(proposal);
    }
  };

  class Phase2 : public ProposalPhaser{
  public:
    explicit Phase2(Proposer* proposer):
      ProposalPhaser(proposer){}
    ~Phase2() override = default;

    bool ExecutePhase(Proposal* proposal) override{
      if(!StartTimeoutTimer()){
        DLOG(WARNING) << "stopping phase2, cannot start timeout timer.";
        return false;
      }

      //TODO:
      // - state check
      ProposalScope::SetPhase(ProposalPhase::kPhase2);
      PeerSessionManager::BroadcastCommit();
      ProposalScope::Accepted(UUID()); //TODO: get node_id
      WaitForRequiredVotes();

      if(TimedOut()){
        return TimeoutProposal();
      } else if(!ProposalScope::HasRequiredVotes()){
        return RejectProposal();
      }

      DLOG(INFO) << "phase2 has finished.";

      auto& queue = proposer()->GetRuntime()->GetTaskQueue();
      auto& engine = proposer()->GetRuntime()->GetTaskEngine();
      atomic::LinkedList<leveldb::WriteBatch> write_queue;

      auto start = Clock::now();
      auto blk = Block::Genesis();//TODO: get proposal block
      auto& transactions = blk->transactions();
      for(auto& it : transactions){
        auto task = new task::ProcessTransactionTask(&engine, write_queue, it);
        if(!queue.Push(reinterpret_cast<uword>(task))){
          LOG(FATAL) << "cannot submit new task to task queue.";
          return false;
        }

        DLOG(INFO) << "waiting for task to finish.";
        while(!task->IsFinished());//spin
        DLOG(INFO) << "done waiting.";
      }

      DLOG(INFO) << "compiling batch.....";
      DLOG(INFO) << "processing " << write_queue.size() << " writes....";
      leveldb::WriteBatch batch;
      while(!write_queue.empty()){
        auto next = write_queue.pop_front();
        DVLOG(2) << "appending batch of size " << next->ApproximateSize() << "b....";
        batch.Append((*next));
      }

      auto end = Clock::now();
      DLOG(INFO) << "batch compiled " << batch.ApproximateSize() << "b!";

      auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
      DLOG(INFO) << "processing took " << duration_ms.count() << "ms";

      auto& pool = proposer()->GetRuntime()->GetPool();
      leveldb::Status status;
      if(!(status = pool.Write(batch)).ok()){
        LOG(FATAL) << "cannot write batch to pool.";
        return false;
      }
      return true;
    }

    static inline bool
    Execute(Proposer* proposer, Proposal* proposal){
      Phase2 phase(proposer);
      return phase.ExecutePhase(proposal);
    }
  };

  void Proposer::HandleOnStart(uv_async_t* handle){ //TODO: better error handling & logging
    auto proposer = (Proposer*)handle->data;
    auto height = 0;
    auto hash = Hash();
    auto proposal = ProposalScope::CreateNewProposal(height, hash);
    DLOG(INFO) << "starting proposal " << proposal->ToString() << "....";
    if(!proposer->ExecutePhase1())
      DLOG(FATAL) << "cannot execute proposal phase1.";
  }

  void Proposer::HandleOnPhase1(uv_async_t* handle){
    auto proposer = (Proposer*)handle->data;
    auto& pool = proposer->GetRuntime()->GetPool();
    Proposal* proposal = nullptr;

    if(!Phase1::Execute(proposer, proposal)){
      DLOG(FATAL) << "cannot execute proposal phase1.";
      return;
    }

    DLOG(INFO) << "proposal phase1 has finished.";
    if(!proposer->ExecutePhase2())
      DLOG(FATAL) << "cannot execute proposal phase2.";
  }

  void Proposer::HandleOnPhase2(uv_async_t* handle){
    auto proposer = (Proposer*)handle->data;
    auto& pool = proposer->GetRuntime()->GetPool();
    Proposal* proposal = nullptr;

    if(!Phase2::Execute(proposer, proposal)){
      DLOG(FATAL) << "cannot execute proposal phase2.";
      return;
    }

    DLOG(INFO) << "proposal phase2 has finished.";
    if(!proposer->OnAccepted())
      DLOG(FATAL) << "cannot finish accepting proposal.";
  }

  void Proposer::HandleOnAccepted(uv_async_t* handle){
    auto proposer = (Proposer*)handle->data;
    DLOG(INFO) << "proposal was accepted by " << ProposalScope::GetRequiredVotes() << " peers.";
  }

  void Proposer::HandleOnRejected(uv_async_t* handle){
    //TODO: cleanup logging
    DLOG(INFO) << "proposal was rejected by " << ProposalScope::GetRequiredVotes() << " peers.";
  }

  void Proposer::HandleOnTimeout(uv_async_t* handle){
    DLOG(INFO) << "proposal timed out.";
  }
}