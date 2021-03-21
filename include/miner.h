#ifndef TOKEN_MINER_H
#define TOKEN_MINER_H

#include <mutex>
#include <ostream>
#include <condition_variable>

#include "vthread.h"
#include "consensus/proposal.h"
#include "peer/peer_session_manager.h"

namespace token{
#define LOG_MINER(LevelName) \
  LOG(LevelName) << "[miner] "

#define LOG_MINER_IF(LevelName, Condition) \
  LOG_IF(LevelName, Condition) << "[miner] "

#define DLOG_MINER(LevelName) \
  DLOG(LevelName) << "[miner] "

#define DLOG_MINER_IF(LevelName, Condition) \
  DLOG_IF(LevelName, Condition) << "[miner] "

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
      return PeerSessionManager::GetNumberOfConnectedPeers();
    }

    static void OnMine(uv_timer_t* handle);

    static void OnPromise(uv_async_t* handle){
      DLOG_MINER(INFO) << "OnPromise called.";
    }
   public:
    explicit BlockMiner(uv_loop_t* loop=uv_loop_new()):
      loop_(loop),
      timer_(),
      state_(State::kStoppedState),
      proposal_mtx_(),
      proposal_(){
      timer_.data = this;
    }
    ~BlockMiner() = default;

    State GetState() const{
      return (State)state_;
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

    bool RegisterNewProposal(const RawProposal& proposal){
      std::lock_guard<std::mutex> guard(proposal_mtx_);
      if(proposal_)
        return false;
      proposal_ = Proposal::NewInstance(loop_, proposal, GetRequiredVotes());
      return true;
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
    inline bool Is##Name() const{ return ((State)state_)== State::k##Name##State; }
    FOR_EACH_MINER_STATE(DEFINE_CHECK)
#undef DEFINE_CHECK

    static inline const char*
    GetThreadName(){
      return "miner";
    }

    static BlockMiner* GetInstance();
  };

  class BlockMinerThread{
   private:
    static void HandleThread(uword param);
   public:
    BlockMinerThread() = delete;
    ~BlockMinerThread() = delete;

    static bool Stop();
    static bool Start();
  };
}

#endif//TOKEN_MINER_H