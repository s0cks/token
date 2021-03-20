#ifndef TOKEN_PROPOSAL_PHASE_H
#define TOKEN_PROPOSAL_PHASE_H

namespace token{
#define FOR_EACH_PROPOSAL_PHASE(V) \
  V(Queued)                        \
  V(Prepare)                      \
  V(Commit)

  enum ProposalPhase{
#define DEFINE_PHASE(Name) k##Name##Phase,
    FOR_EACH_PROPOSAL_PHASE(DEFINE_PHASE)
#undef DEFINE_PHASE
  };

  static std::ostream& operator<<(std::ostream& stream, const ProposalPhase& phase){
    switch(phase){
#define DEFINE_TOSTRING(Name) \
      case ProposalPhase::k##Name##Phase: \
        return stream << #Name;
      FOR_EACH_PROPOSAL_PHASE(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
      default:
        return stream << "Unknown";
    }
  }
}

#endif//TOKEN_PROPOSAL_PHASE_H