#ifndef TOKEN_MINER_H
#define TOKEN_MINER_H

#include <mutex>
#include <ostream>
#include <condition_variable>

#include "vthread.h"
#include "proposal.h"
#include "peer/peer_session_manager.h"

namespace token{
#define FOR_EACH_MINER_STATE(V) \
  V(Starting)                   \
  V(Running)                    \
  V(Stopping)                   \
  V(Stopped)

  class BlockMiner{
   public:
    enum State{
#define DEFINE_STATE(Name) k##Name##State,
      FOR_EACH_MINER_STATE(DEFINE_STATE)
#undef DEFINE_STATE
    };

    friend std::ostream& operator<<(std::ostream& stream, const State& state){
      switch(state){
#define DEFINE_TOSTRING(Name) \
        case State::k##Name##State: \
          return stream << #Name;
        FOR_EACH_MINER_STATE(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
        default:
          return stream << "Unknown";
      }
    }
   protected:
    uv_loop_t* loop_;
    uv_timer_t timer_;
    RelaxedAtomic<State> state_;

    std::mutex proposal_mtx_;
    ProposalPtr proposal_;

    void SetState(const State& state){
      state_ = state;
    }

    bool StopMiningTimer();
    bool StartMiningTimer();

    static inline int16_t
    GetRequiredVotes(){
#ifdef TOKEN_ENABLE_SERVER
      return PeerSessionManager::GetNumberOfConnectedPeers();
#else
      return 0;
#endif//TOKEN_ENABLE_SERVER
    }

    static void OnMine(uv_timer_t* handle);
   public:
    BlockMiner(uv_loop_t* loop=uv_loop_new()):
      loop_(loop),
      timer_(),
      state_(State::kStoppedState),
      proposal_mtx_(),
      proposal_(){
      timer_.data = this;
    }
    ~BlockMiner() = default;

    State GetState() const{
      return state_;
    }

    uv_loop_t* GetLoop() const{
      return loop_;
    }

    ProposalPtr GetActiveProposal(){
      std::lock_guard<std::mutex> guard(proposal_mtx_);
      return proposal_;
    }

    bool HasActiveProposal(){
      std::lock_guard<std::mutex> guard(proposal_mtx_);
      return proposal_ != nullptr;
    }

    bool IsActiveProposal(const RawProposal& proposal){
      std::lock_guard<std::mutex> guard(proposal_mtx_);
      if(!proposal_)
        return false;
      return proposal_->raw() == proposal;
    }

    bool SetActiveProposal(const ProposalPtr& proposal){
      std::lock_guard<std::mutex> guard(proposal_mtx_);
      if(proposal_)
        return false;
      proposal_ = proposal;
      return true;
    }

    bool SetActiveProposal(const RawProposal& proposal){
      std::lock_guard<std::mutex> guard(proposal_mtx_);
      if(proposal_)
        return false;
      proposal_ = Proposal::NewInstance(nullptr, loop_, proposal, GetRequiredVotes());
      return true;
    }

    bool RegisterNewProposal(const RawProposal& proposal){
      std::lock_guard<std::mutex> guard(proposal_mtx_);
      if(proposal_)
        return false;
      proposal_ = Proposal::NewInstance(nullptr, loop_, proposal, GetRequiredVotes());
      return proposal_->TransitionToPhase(ProposalPhase::kPreparePhase);
    }

    bool ClearActiveProposal(){
      std::lock_guard<std::mutex> guard(proposal_mtx_);
      if(!proposal_)
        return false;
      proposal_ = nullptr;
      return true;
    }

    bool Run();

    inline bool Pause(){
      return StopMiningTimer();
    }

    inline bool Resume(){
      return StartMiningTimer();
    }

#define DEFINE_CHECK(Name) \
    inline bool Is##Name() const{ return state_ == State::k##Name##State; }
    FOR_EACH_MINER_STATE(DEFINE_CHECK)
#undef DEFINE_CHECK

    static BlockMiner* GetInstance();
  };

  class BlockMinerThread{
   private:
    BlockMinerThread() = delete;

    static void HandleThread(uword param);
   public:
    ~BlockMinerThread() = delete;

    static bool Stop();
    static bool Start();
    static void Initialize();
  };
}

#endif//TOKEN_MINER_H