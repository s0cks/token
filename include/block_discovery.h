#ifndef TOKEN_BLOCK_DISCOVERY_H
#define TOKEN_BLOCK_DISCOVERY_H

#include "vthread.h"

namespace Token{
    class BlockDiscoveryThread : public Thread{
        //TODO:
        // - need to pause block discovery thread when proposal thread is running or merge these 2 together again
    private:
        BlockDiscoveryThread() = delete;

        static void SetState(Thread::State state);
        static void HandleThread(uword parameter);
    public:
        ~BlockDiscoveryThread() = delete;

        static Thread::State GetState();
        static void WaitForState(Thread::State state);

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

        static bool Start(){
            //TODO: fix parameter
            return Thread::Start("BlockDiscoveryThread", &HandleThread, 0);
        }

        static bool Pause(){
            if(!IsRunning()) return false;
            LOG(INFO) << "pausing block discovery thread....";
            SetState(Thread::State::kPaused);
            return true;
        }

        static bool Stop(){
            //TODO: implement
            return false;
        }
    };
}

#endif //TOKEN_BLOCK_DISCOVERY_H