#ifndef TOKEN_PROPOSER_H
#define TOKEN_PROPOSER_H

#include "vthread.h"
#include "proposal.h"

namespace Token{
    class ProposerThread : public Thread{
    private:
        ProposerThread() = delete;

        static void SetState(Thread::State state);
        static void HandleThread(uword parameter);
        static void HandleVotingPhase(const Handle<Proposal>& proposal);
        static void HandleCommitPhase(const Handle<Proposal>& proposal);
        static void HandleQuorumPhase(const Handle<Proposal>& proposal);
    public:
        ~ProposerThread() = delete;

        static void WaitForState(Thread::State state);
        static Thread::State GetState();
        static bool HasProposal();
        static bool SetProposal(const Handle<Proposal>& proposal);
        static Handle<Proposal> GetProposal();

        static bool Start(){
            //TODO: fix parameter
            return Thread::Start("ProposerThread", &HandleThread, 0) == 0;
        }

        static bool
        IsRunning(){
            return GetState() == Thread::State::kRunning;
        }

        static bool
        IsPaused(){
            return GetState() == Thread::State::kPaused;
        }

        static bool
        IsStopped(){
            return GetState() == Thread::State::kStopped;
        }
    };
}

#endif //TOKEN_PROPOSER_H