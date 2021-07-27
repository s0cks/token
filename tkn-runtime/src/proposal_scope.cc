#include <mutex>
#include "proposal_scope.h"
#include "atomic/relaxed_atomic.h"
#include "peer/peer_session_manager.h"

namespace token{
  typedef atomic::RelaxedAtomic<uint64_t> AtomicCounter;

  static std::mutex mtx_current_;//TODO: remove
  static Proposal* current_ = nullptr;
  static AtomicCounter promises_(0);
  static AtomicCounter accepted_(0);
  static AtomicCounter rejected_(0);
  static atomic::RelaxedAtomic<ProposalPhase> phase_(ProposalPhase::kQueued);

  Proposal* ProposalScope::CreateNewProposal(const int64_t& height, const Hash& hash){//TODO: remove locking
    std::lock_guard<std::mutex> guard(mtx_current_);
    Timestamp timestamp = Clock::now();
    UUID proposal_id = UUID();
    UUID proposer_id = UUID();//TODO: get node id
    return current_ = new Proposal(timestamp, proposal_id, proposer_id, height, hash);
  }

  Proposal* ProposalScope::GetCurrentProposal(){//TODO: remove locking
    std::lock_guard<std::mutex> guard(mtx_current_);
    return current_;
  }

  bool ProposalScope::HasActiveProposal(){//TODO: remove locking
    std::lock_guard<std::mutex> guard(mtx_current_);
    return current_ != nullptr;
  }

  bool ProposalScope::SetActiveProposal(const Proposal& proposal){//TODO: remove locking
    std::lock_guard<std::mutex> guard(mtx_current_);
    current_ = new Proposal(proposal);
    return true;
  }

  void ProposalScope::SetPhase(const ProposalPhase& phase){
    phase_ = phase;
  }

  ProposalPhase ProposalScope::GetPhase(){
    return (ProposalPhase)phase_;
  }

  uint64_t ProposalScope::GetRequiredVotes(){
    switch(GetPhase()){
      case ProposalPhase::kPhase1:
      case ProposalPhase::kPhase2:
        return ((PeerSessionManager::GetNumberOfConnectedPeers() / 2) + 1);//TODO: investigate fair calculation
      case ProposalPhase::kQuorum:
      default:
        return 0;
    }
  }

  void ProposalScope::Promise(const UUID& node_id){
    DLOG(INFO) << node_id << " promised the proposal.";
    promises_ += 1;
  }

  uint64_t ProposalScope::GetPromises(){
    return (uint64_t)promises_;
  }

  void ProposalScope::Accepted(const UUID& node_id){
    DLOG(INFO) << node_id << " accepted the proposal.";
    accepted_ += 1;
  }

  uint64_t ProposalScope::GetAccepted(){
    return (uint64_t)accepted_;
  }

  void ProposalScope::Rejected(const UUID& node_id){
    DLOG(INFO) << node_id << " rejected the proposal.";
  }

  uint64_t ProposalScope::GetRejected(){
    return (uint64_t)rejected_;
  }
}