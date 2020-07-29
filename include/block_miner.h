#ifndef TOKEN_BLOCK_MINER_H
#define TOKEN_BLOCK_MINER_H

#include <pthread.h>
#include <uv.h>
#include "common.h"
#include "block.h"
#include "proposal.h"

namespace Token{
    //TODO:
    // - refactor mining logic
    // - add timeout to proposal logic
    class BlockMiner{
        friend class BlockChain;
        friend class Server;
        friend class PeerSession;
    public:
        enum State{
            kStarting,
            kRunning,
            kPaused,
            kStopped,
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

        static const uint32_t kMiningIntervalMilliseconds = 1 * 1000;
        static const uint32_t kNumberOfTransactionsPerBlock = 2;
    private:
        BlockMiner() = delete;

        static void SetState(State state);
        static void HandleTerminateCallback(uv_async_t* handle);
        static void HandleProposalTimeoutCallback(uv_timer_t* handle);
        static void HandleMineBlockCallback(uv_timer_t* handle);
        static void* MinerThread(void* data);

        static inline Proposal*
        CreateNewProposal(Block* block, const std::string& node_id=Server::GetID()){
            Proposal* proposal = Proposal::NewInstance(block, node_id);
            SetProposal(proposal);
            return proposal;
        }
    public:
        ~BlockMiner() = delete;

        static State GetState();
        static Handle<Proposal> GetProposal();
        static bool HasProposal();
        static void SetProposal(const Handle<Proposal>& proposal);
        static void Initialize();
        static void Pause();
        static void Resume();
        static void Shutdown();
        static void WaitForState(State state);

        static inline bool
        IsRunning(){
            return GetState() == kRunning;
        }

        static inline bool
        IsStopped(){
            return GetState() == kStopped;
        }

        static inline bool
        IsPaused(){
            return GetState() == kPaused;
        }
    };
}

#endif //TOKEN_BLOCK_MINER_H