#ifndef TOKEN_BLOCK_MINER_H
#define TOKEN_BLOCK_MINER_H

#include <pthread.h>
#include <uv.h>
#include "common.h"
#include "block.h"

namespace Token{
    //TODO:
    // - establish timer for mining
    // - refactor
    class BlockMiner{
    public:
        enum State{
            kStarting,
            kRunning,
            kPaused,
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
        static const uint32_t kMiningIntervalMilliseconds = 1 * 1000;
        static const uint32_t kNumberOfTransactionsPerBlock = 20000;

        ~BlockMiner(){}

        static State GetState();
        static void Initialize();
        static void Pause();
        static void Resume();
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

        static inline bool
        IsPaused(){
            return GetState() == kPaused;
        }
    };
}

#endif //TOKEN_BLOCK_MINER_H