#ifndef TKN_PROPOSAL_ELECTION_H
#define TKN_PROPOSAL_ELECTION_H

#include <uv.h>

#include "flags.h"
#include "common.h"
#include "../../../Sources/token/proposal.h"
#include "proposal_state.h"
#include "atomic/relaxed_atomic.h"
#include "peer/peer_session_manager.h"

namespace token{
  typedef atomic::RelaxedAtomic<uint32_t> ElectionCounter;

#define FOR_EACH_ELECTION_STATE(V) \
  V(InProgress)                    \
  V(Quorum)

#define FOR_EACH_ELECTION_EVENT(V) \
  V(Start, "election.start", on_start_)      \
  V(Pass, "election.pass", on_pass_)                \
  V(Fail, "election.fail", on_fail_)                \
  V(Timeout, "election.timeout", on_timeout_)          \
  V(Finished, "election.finished", on_finished_)

  class ElectionEventListener{
  protected:
    uv_async_t on_start_;
    uv_async_t on_pass_;
    uv_async_t on_fail_;
    uv_async_t on_timeout_;
    uv_async_t on_finished_;

    explicit ElectionEventListener(uv_loop_t* loop, EventBus& bus):
      on_start_(),
      on_pass_(),
      on_fail_(),
      on_timeout_(),
      on_finished_(){
#define INITIALIZE_EVENT_HANDLE(Name, Event, Handle) \
      (Handle).data = this;             \
      CHECK_UVRESULT2(FATAL, uv_async_init(loop, &(Handle), [](uv_async_t* handle){ \
        return ((ElectionEventListener*)handle->data)->HandleOnElection##Name();                    \
      }), "cannot initialize the callback handle");                                 \
      bus.Subscribe(Event, &(Handle));
      FOR_EACH_ELECTION_EVENT(INITIALIZE_EVENT_HANDLE)
#undef INITIALIZE_EVENT_HANDLE
    }

#define DECLARE_EVENT_HANDLER(Name, Event, Handle) \
    virtual void HandleOnElection##Name() = 0;
    FOR_EACH_ELECTION_EVENT(DECLARE_EVENT_HANDLER)
#undef DECLARE_EVENT_HANDLER
  public:
    virtual ~ElectionEventListener() = default;
  };

#define DECLARE_ELECTION_EVENT_HANDLER(Name, Event, Handle) \
  void HandleOnElection##Name() override;            \

#define DEFINE_ELECTION_EVENT_LISTENER \
  FOR_EACH_ELECTION_EVENT(DECLARE_ELECTION_EVENT_HANDLER)

  class Runtime;
  class Election{
  public:
    enum State{
#define DEFINE_STATE(Name) k##Name,
      FOR_EACH_ELECTION_STATE(DEFINE_STATE)
#undef DEFINE_STATE
    };

    inline friend std::ostream&
    operator<<(std::ostream& stream, const State& val){
      switch(val){
#define DEFINE_TOSTRING(Name) \
        case State::k##Name: return stream << #Name;
        FOR_EACH_ELECTION_STATE(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
        default:
          return stream << "Unknown";
      }
    }

    static inline uint64_t
    GetElectionTimeoutMilliseconds(){
      return FLAGS_proposal_timeout/4;
    }

    static inline uint64_t
    GetElectionPollerIntervalMilliseconds(){
      // ex:
      // proposal_timeout := 1000*60*10 (600000)
      // election_timeout := $proposal_timeout/4 (600000/4 := 150000)
      // election_poller_interval := $election_timeout/1000 (150000/1000 := 150)
      return GetElectionTimeoutMilliseconds()/1000;
    }

    static inline uint32_t
    GetElectionRequiredVotes(){
      auto peers = PeerSessionManager::GetNumberOfConnectedPeers();
      auto votes = static_cast<uint32_t>(std::ceil(peers / 2))+1;
      return votes;
    }
  protected:
    Runtime* runtime_;
    ProposalState::Phase phase_;
    atomic::RelaxedAtomic<State> state_;
    uv_timer_t timeout_;
    uv_timer_t poller_;
    uint32_t required_;
    ElectionCounter accepted_;
    ElectionCounter rejected_;
    Timestamp ts_start_;
    Timestamp ts_finished_;

    void SetState(const State& state){
      state_ = state;
    }

    inline bool
    StartPoller(){
      auto callback = [](uv_timer_t* handle){
        return ((Election*)handle->data)->OnPoll();
      };
      VERIFY_UVRESULT2(FATAL, uv_timer_start(&poller_, callback, 0, GetElectionPollerIntervalMilliseconds()), "cannot start the election poller");
      return true;
    }

    inline bool
    StopPoller(){
      VERIFY_UVRESULT2(FATAL, uv_timer_stop(&poller_), "cannot stop the election poll");
      return true;
    }

    inline bool
    StartTimeoutTimer(){
      auto callback = [](uv_timer_t* handle){
        return ((Election*)handle->data)->OnTimeout();
      };
      VERIFY_UVRESULT2(FATAL, uv_timer_start(&timeout_, callback, GetElectionTimeoutMilliseconds(), 0), "cannot start the election timeout timer");
      return true;
    }

    inline bool
    StopTimeoutTimer(){
      VERIFY_UVRESULT2(FATAL, uv_timer_stop(&timeout_), "cannot stop the election timeout timer");
      return true;
    }

    inline void
    SetStartTime(const Timestamp& val=Clock::now()){
      ts_start_ = val;
    }

    inline void
    SetFinishTime(const Timestamp& val=Clock::now()){
      ts_finished_ = val;
    }

    void PassElection();
    void FailElection();
    void OnPoll();
    void OnTimeout();
  public:
    Election(Runtime* runtime, const ProposalState::Phase& phase);
    virtual ~Election() = default;

    ProposalState::Phase GetPhase() const{
      return phase_;
    }

    bool StartElection();
    bool StopElection();

    Timestamp GetStartTime() const{
      return ts_start_;
    }

    Timestamp GetFinishTime() const{
      return ts_finished_;
    }

    State GetState() const{
      return (State)state_;
    }

    uint32_t accepted() const{
      return (uint32_t)accepted_;
    }

    uint32_t rejected() const{
      return (uint32_t)rejected_;
    }

    uint32_t current_votes() const{
      return accepted() + rejected();
    }

    uint32_t required_votes() const{
      return required_;
    }

    bool HasRequiredVotes() const{
      return current_votes() > required_votes();
    }

    void Accept(const UUID& node_id){
      DLOG(INFO) << "proposal was accepted by " << node_id;
      accepted_ += 1;
    }

    void Reject(const UUID& node_id){
      DLOG(INFO) << "proposal was rejected by " << node_id;
      rejected_ += 1;
    }

    void Reset(){
      accepted_ = 0;
      rejected_ = 0;
    }

    inline double
    GetAcceptedPerc() const{
      return (static_cast<double>(accepted()) / static_cast<double>(current_votes())) * 100.0;
    }

    inline double
    GetRejectedPerc() const{
      return (static_cast<double>(rejected()) / static_cast<double>(current_votes())) * 100.0;
    }

    inline double
    GetVotePercentage() const{
      return ((static_cast<double>(current_votes())/static_cast<double>(required_votes()))*100.0);
    }

    inline uint64_t
    GetElectionElapsedTimeMilliseconds() const{
      return GetElapsedTimeMilliseconds(GetStartTime(), Clock::now());
    }

    inline uint64_t
    GetElectionTimeRemainingMilliseconds () const{
      return GetElectionTimeoutMilliseconds()-GetElectionElapsedTimeMilliseconds();
    }

    inline uint64_t
    GetElectionTimeMilliseconds() const{
      return GetElapsedTimeMilliseconds(GetStartTime(), GetFinishTime());
    }

#define DEFINE_STATE_CHECK(Name) \
    inline bool Is##Name() const{ return GetState() == State::k##Name; }
    FOR_EACH_ELECTION_STATE(DEFINE_STATE_CHECK)
#undef DEFINE_STATE_CHECK
  };
}

#endif//TKN_PROPOSAL_ELECTION_H