#ifndef TOKEN_JOB_POOL_WORKER_H
#define TOKEN_JOB_POOL_WORKER_H

#include "task/job.h"

namespace Token{
#define FOR_EACH_JOB_POOL_WORKER_STATE(V) \
    V(Starting)                           \
    V(Idle)                               \
    V(Running)                            \
    V(Stopping)                           \
    V(Stopped)

    class JobPoolWorker{
    public:
        enum State{
#define DEFINE_STATE(Name) k##Name,
            FOR_EACH_JOB_POOL_WORKER_STATE(DEFINE_STATE)
#undef DEFINE_STATE
        };

        friend std::ostream& operator<<(std::ostream& stream, const State& state){
            switch(state){
#define DEFINE_TOSTRING(Name) \
                case State::k##Name: \
                    stream << #Name; \
                    return stream;
                FOR_EACH_JOB_POOL_WORKER_STATE(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
                default:
                    stream << "Unknown";
                    return stream;
            }
        }
    private:
        JobQueue* queue_;
        pthread_t thread_;
        std::mutex mutex_;
        std::condition_variable cond_;
        State state_;

        void SetState(const State& state){
            std::unique_lock<std::mutex> lock(mutex_);
            state_ = state;
            cond_.notify_all();
        }

        static void* HandleThread(void* data){
            LOG(INFO) << "starting worker....";

            JobPoolWorker* worker = (JobPoolWorker*)data;
            worker->SetState(JobPoolWorker::kRunning);
            while(worker->IsRunning()){
                Job* next = worker->queue_->Steal();
                if(next){
                    if(!next->Run()){
                        LOG(WARNING) << "couldn't run the \"" << next->GetName() << "\" job.";
                        continue;
                    }
                    LOG(INFO) << next->GetName() << " finished run w/ the following result: " << next->GetResult();
                }
            }

            return nullptr;
        }
    public:
        JobPoolWorker(JobQueue* queue):
            queue_(queue),
            thread_(),
            mutex_(),
            cond_(),
            state_(State::kStarting){}
        ~JobPoolWorker() = default;

        State GetState(){
            std::unique_lock<std::mutex> lock(mutex_);
            return state_;
        }

        bool Start(){
            int result;
            pthread_attr_t thread_attr;
            if((result = pthread_attr_init(&thread_attr)) != 0){
                LOG(WARNING) << "couldn't initialize the thread attributes: " << strerror(result);
                return false;
            }

            if((result = pthread_create(&thread_, &thread_attr, &HandleThread, this)) != 0){
                LOG(WARNING) << "couldn't start the session thread: " << strerror(result);
                return false;
            }

            if((result = pthread_attr_destroy(&thread_attr)) != 0){
                LOG(WARNING) << "couldn't destroy the thread attributes: " << strerror(result);
                return false;
            }
            return true;
        }

        bool Stop(){
            SetState(JobPoolWorker::kStopping);

            void** result = NULL;
            int err;
            if((err = pthread_join(thread_, result)) != 0){
                LOG(WARNING) << "couldn't join thread: " << strerror(err);
                return false;
            }
            return true;
        }

#define DEFINE_CHECK(Name) \
        bool Is##Name(){ return GetState() == State::k##Name; }
        FOR_EACH_JOB_POOL_WORKER_STATE(DEFINE_CHECK)
#undef DEFINE_CHECK
    };
}

#endif //TOKEN_JOB_POOL_WORKER_H