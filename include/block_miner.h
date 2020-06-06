#ifndef TOKEN_BLOCK_MINER_H
#define TOKEN_BLOCK_MINER_H

#include <pthread.h>
#include <uv.h>
#include "common.h"
#include "paxos.h"

namespace Token{
    //TODO:
    // - establish timer for mining
    // - refactor
    class BlockMiner{
    private:
        static Proposal* GetProposal();
        static void SetProposal(Proposal* proposal);
        static bool HasProposal();
        static bool SubmitProposal(Proposal* proposal);
        static bool CommitProposal(Proposal* proposal);
        static bool VoteForProposal(const std::string& node_id);
        static bool AcceptProposal(const std::string& node_id);

        static void HandleMineCallback(uv_timer_t* handle);
        static void HandleExitCallback(uv_async_t* handle);
        static void* MinerThread(void* data);

        BlockMiner(){}

        friend class BlockChain;
        friend class Node;
        friend class PeerSession;
    public:
        static const uint64_t kMiningIntervalMilliseconds = 1 * 1000;

        ~BlockMiner(){}

        static bool Initialize();
    };
}

#endif //TOKEN_BLOCK_MINER_H