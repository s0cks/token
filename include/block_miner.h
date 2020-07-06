#ifndef TOKEN_BLOCK_MINER_H
#define TOKEN_BLOCK_MINER_H

#include <pthread.h>
#include <uv.h>
#include "common.h"
#include "proposal.h"

namespace Token{
    //TODO:
    // - establish timer for mining
    // - refactor
    class BlockMiner{
    public:
        enum State{
            kStarting,
            kRunning,
            kStopped,
            // kMining - mining a block or working a proposal
        };

        friend std::ostream& operator<<(std::ostream& stream, const State& state){
            switch(state){
                case kRunning:
                    stream << "Running";
                    break;
                case kStopped:
                    stream << "Stopped";
                    break;
                default:
                    stream << "Unknown";
                    break;
            }
            return stream;
        }
    private:
        static void SetState(State state);

        //TODO: refactor proposal system to be more fluid + stateful
        static Proposal* GetProposal();
        static bool HasProposal();
        static bool SubmitProposal(Proposal* proposal);
        static bool CommitProposal(Proposal* proposal);
        static bool VoteForProposal(const std::string& node_id);
        static bool AcceptProposal(const std::string& node_id);

        static bool MineBlock(Block* block, bool clean);
        static void WaitForState(State state);
        static void HandleTerminateCallback(uv_async_t* handle);
        static void HandleMineCallback(uv_timer_t* handle);
        static void* MinerThread(void* data);

        BlockMiner(){}

        friend class BlockChain;
        friend class Node;
        friend class PeerSession;
    public:
        static const uint64_t kMiningIntervalMilliseconds = 1 * 1000;

        ~BlockMiner(){}

        static State GetState();
        static void Initialize();
        static void SetProposal(Proposal* proposal);
        static bool Shutdown();

        //TODO: refactor
        static inline void WaitForRunningState(){
            WaitForState(State::kRunning);
        }

        //TODO: refactor
        static inline void WaitForStoppedState(){
            WaitForState(State::kStopped);
        }

        static inline bool
        IsRunning(){
            return GetState() == kRunning;
        }

        static inline bool
        IsStopped(){
            return GetState() == kStopped;
        }
    };
}

#endif //TOKEN_BLOCK_MINER_H