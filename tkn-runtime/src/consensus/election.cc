#include "runtime.h"
#include "consensus/election.h"

namespace token{
  Election::Election(Runtime* runtime, const ProposalState::Phase& phase):
    runtime_(runtime),
    phase_(phase),
    state_(State::kQuorum),
    timeout_(),
    poller_(),
    required_(GetElectionRequiredVotes()),
    accepted_(0),
    rejected_(0),
    ts_start_(),
    ts_finished_(){
    timeout_.data = this;
    CHECK_UVRESULT2(FATAL, uv_timer_init(runtime->loop(), &timeout_), "cannot initialize the election timeout timer handle");
    poller_.data = this;
    CHECK_UVRESULT2(FATAL, uv_timer_init(runtime->loop(), &poller_), "cannot initialize the election poller timer handle");
  }

  void Election::OnPoll(){
    uint32_t current = current_votes();
    uint32_t required = required_votes();
    DVLOG(1) << "polling for votes in election (" << GetVotePercentage() << "%; " << current << "/" << required << "; " << GetElectionElapsedTimeMilliseconds() << "ms)";
    if(current >= required){
      return accepted() > rejected()
             ? PassElection()
             : FailElection();
    }
  }

  void Election::PassElection(){
    runtime_->OnElectionPass();
    if(!StopElection())
      DLOG(FATAL) << "cannot stop the election.";
  }

  void Election::FailElection(){
    runtime_->OnElectionFail();
    if(!StopElection())
      DLOG(FATAL) << "cannot stop the election.";
  }

  void Election::OnTimeout(){
    DLOG(ERROR) << "the election has timed out.";
    runtime_->OnElectionTimeout();
    if(!StopElection())
      DLOG(FATAL) << "cannot stop the election.";
  }

  bool Election::StartElection(){
    if(IsInProgress()){
      DLOG(ERROR) << "the election is in-progress, cannot start.";
      return false;
    }
    if(!StartPoller()){
      DLOG(ERROR) << "cannot start the election poller";
      return false;
    }
    if(!StartTimeoutTimer()){
      DLOG(ERROR) << "cannot start the election timeout timer";
      return false;
    }
    runtime_->GetProposalState().SetPhase(GetPhase());
    SetState(State::kInProgress);
    SetStartTime();
    runtime_->OnElectionStart();
    DVLOG(1) << "the election has started. (timeout=" << GetElectionTimeoutMilliseconds() << "ms, required_votes=" << GetElectionRequiredVotes() << ", poll=" << GetElectionPollerIntervalMilliseconds() << "ms)";
    return true;
  }

  bool Election::StopElection(){
    if(!IsInProgress()){
      DLOG(ERROR) << "the election is not in-progress.";
      return false;
    }
    if(!StopPoller()){
      DLOG(ERROR) << "cannot stop the election poller.";
      return false;
    }

    if(!StopTimeoutTimer()){
      DLOG(ERROR) << "cannot stop the election timeout.";
      return false;
    }
    SetState(State::kQuorum);
    SetFinishTime();
    runtime_->OnElectionFinished();
    return true;
  }
}