#ifndef TOKEN_PROPOSER_H
#define TOKEN_PROPOSER_H

#include "vthread.h"
#include "proposal.h"

namespace Token{
    class ProposerThread : public Thread{
    private:
        ProposerThread() = delete;

        static void HandleThread(uword parameter);
    public:
        ~ProposerThread() = delete;

        static bool HasProposal();
        static bool SetProposal(const Handle<Proposal>& proposal);
        static Handle<Proposal> GetProposal();

        static bool Start(){
            //TODO: fix parameter
            return Thread::Start("ProposerThread", &HandleThread, 0) == 0;
        }
    };
}

#endif //TOKEN_PROPOSER_H