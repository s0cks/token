#ifndef TOKEN_JOB_POOL_WORKER_H
#define TOKEN_JOB_POOL_WORKER_H

#include "task/job.h"
#include "utils/metrics.h"

namespace Token{
#define FOR_EACH_JOB_POOL_WORKER_STATE(V) \
    V(Starting)                           \
    V(Idle)                               \
    V(Running)                            \
    V(Stopping)                           \
    V(Stopped)

    class JobPoolWorker{
        friend class JobPool;
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
        pthread_t thread_;
        std::mutex mutex_;
        std::condition_variable cond_;
        State state_;
        JobQueue queue_;
        Counter num_ran_;
        Counter num_discarded_;

        void SetState(const State& state){
            std::lock_guard<std::mutex> lock(mutex_);
            state_ = state;
            cond_.notify_all();
        }

        bool Schedule(Job* job){
            return queue_.Push(job);
        }

        void Wait(Job* job){
            while(!job->IsFinished()){
                Job* j = GetNextJob();
                if(j){
                   j->Run();
                   num_ran_->Increment();
                } else{
                    //TODO off cycle counter
                }
            }
        }

        Job* GetNextJob(){
            Job* job = queue_.Pop();
            if(job){
                return job;
            } else{
                JobPoolWorker* worker = JobPool::GetRandomWorker();
                if(worker == this){ //TODO: fix logic
                    pthread_yield();
                    return nullptr;
                }

                if(!worker){
                    pthread_yield();
                    return nullptr;
                }

                return worker->queue_.Steal();
            }
        }

        static void* HandleThread(void* data){
            LOG(INFO) << "starting worker....";
            JobPoolWorker* worker = (JobPoolWorker*)data;
            worker->SetState(JobPoolWorker::kRunning);
            while(worker->IsRunning()){
                Job* next = worker->GetNextJob();
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
        JobPoolWorker(size_t max_queue_size):
            thread_(),
            mutex_(),
            cond_(),
            state_(State::kStarting),
            queue_(max_queue_size),
            num_ran_(),
            num_discarded_(){}
        ~JobPoolWorker() = default;

        State GetState(){
            std::lock_guard<std::mutex> lock(mutex_);
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