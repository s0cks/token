#include "consensus/proposal_manager.h"

namespace token{
  static std::mutex mutex_;
  static std::condition_variable cond_;
  static ProposalPtr current_;

  bool ProposalManager::HasProposal(){
    std::lock_guard<std::mutex> guard(mutex_);
    return current_ != nullptr;
  }

  bool ProposalManager::ClearProposal(){
    std::lock_guard<std::mutex> guard(mutex_);
    if(!current_)
      return true;
    current_ = ProposalPtr(nullptr);
    return true;
  }

  bool ProposalManager::SetProposal(const ProposalPtr& proposal){
    std::lock_guard<std::mutex> guard(mutex_);
    if(current_)
      return false;
    current_ = proposal;
    return true;
  }

  bool ProposalManager::IsProposalFor(const ProposalPtr& proposal){
    std::lock_guard<std::mutex> guard(mutex_);
    if(!current_)
      return false;
    return current_->Equals(proposal);
  }

  ProposalPtr ProposalManager::GetProposal(){
    std::lock_guard<std::mutex> guard(mutex_);
    return current_;
  }
}