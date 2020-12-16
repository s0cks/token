#ifndef TOKEN_JOB_H
#define TOKEN_JOB_H

#include "task.h"

namespace Token{
#define FOR_EACH_JOB_RESULT_STATUS(V) \
    V(Unscheduled)                    \
    V(Successful)                     \
    V(Failed)                         \
    V(Cancelled)                      \
    V(TimedOut)

    class JobResult{
    public:
        enum Status{
#define DEFINE_STATE(Name) k##Name,
            FOR_EACH_JOB_RESULT_STATUS(DEFINE_STATE)
#undef DEFINE_STATE
        };

        friend std::ostream& operator<<(std::ostream& stream, const Status& status){
            switch(status){
#define DEFINE_TOSTRING(Name) \
                case Status::k##Name: \
                    stream << #Name; \
                    return stream;
                FOR_EACH_JOB_RESULT_STATUS(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
                default:
                    stream << "Unknown";
                    return stream;
            }
        }
    private:
        Status status_;
        std::string message_;
    public:
        JobResult(const Status& status, const std::string& msg):
            status_(status),
            message_(msg){}
        JobResult(const JobResult& result):
            status_(result.status_),
            message_(result.message_){}
        ~JobResult() = default;

        Status GetStatus() const{
            return status_;
        }

        std::string GetMessage() const{
            return message_;
        }

        bool IsUnscheduled() const{
            return GetStatus() == JobResult::kUnscheduled;
        }

        bool IsSuccessful() const{
            return GetStatus() == JobResult::kSuccessful;
        }

        bool IsFailed() const{
            return GetStatus() == JobResult::kFailed;
        }

        bool IsTimedOut() const{
            return GetStatus() == JobResult::kTimedOut;
        }

        bool IsCancelled() const{
            return GetStatus() == JobResult::kCancelled;
        }

        void operator=(const JobResult& result){
            status_ = result.status_;
            message_ = result.message_;
        }

        friend std::ostream& operator<<(std::ostream& stream, const JobResult& result){
            stream << result.GetMessage() << " [" << result.GetStatus() << "]";
            return stream;
        }
    };

    class Job : public Task{
    public:
        static inline JobResult
        Success(const std::string& message){
            return JobResult(JobResult::kSuccessful, message);
        }

        static inline JobResult
        Failed(const std::string& message){
            return JobResult(JobResult::kFailed, message);
        }

        static inline JobResult
        TimedOut(const std::string& message){
            return JobResult(JobResult::kTimedOut, message);
        }

        static inline JobResult
        Cancelled(const std::string& message){
            return JobResult(JobResult::kCancelled, message);
        }
    protected:
        Job* parent_;
        std::string name_;
        JobResult result_;
        std::atomic_size_t unfinished_jobs_;

        Job(Job* parent, const std::string& name):
            parent_(parent),
            name_(name),
            result_(JobResult::kUnscheduled, "Unscheduled"),
            unfinished_jobs_(1){
            if(parent != nullptr)
                parent->unfinished_jobs_++;
        }

        virtual JobResult DoWork() = 0;

        void Finish(){
            unfinished_jobs_--;
            if(IsFinished()){
                if(HasParent())
                    GetParent()->Finish();
            }
        }
    public:
        virtual ~Job() = default;

        Job* GetParent() const{
            return parent_;
        }

        bool HasParent() const{
            return parent_ != nullptr;
        }

        bool IsFinished() const{
            return unfinished_jobs_ == 0;
        }

        std::string GetName() const{
            return name_;
        }

        JobResult GetResult() const{
            return result_;
        }

        bool Run(){
            LOG(INFO) << "running " << GetName() << " job....";
            result_ = DoWork();
            Finish();
            return true;
        }
    };

    class JobPool{
    public:
        static const int kMaxNumberOfJobs = 1024;
        static const int kMaxNumberOfWorkers = 4;
    private:
        JobPool() = delete;
    public:
        ~JobPool() = delete;

        static bool Initialize();
        static bool Schedule(Job* job);
    };

    class JobQueue{
    private:
        std::vector<Job*> jobs_;
        std::atomic<size_t> top_;
        std::atomic<size_t> bottom_;
    public:
        JobQueue(size_t max_size):
                jobs_(max_size, nullptr),
                top_(0),
                bottom_(0){}
        ~JobQueue() = default;

        bool Push(Job* job){
            size_t bottom = bottom_.load(std::memory_order_acquire);
            if(bottom < jobs_.size()){
                jobs_[bottom % jobs_.size()] = job;
                std::atomic_thread_fence(std::memory_order_release);
                bottom_.store(bottom + 1, std::memory_order_release);
                return true;
            }
            return false;
        }

        Job* Steal(){
            size_t top = top_.load(std::memory_order_acquire);
            std::atomic_thread_fence(std::memory_order_acquire);
            size_t bottom = bottom_.load(std::memory_order_acquire);
            if(top < bottom){
                Job* job = jobs_[top % jobs_.size()];
                size_t expected_top = top + 1;
                size_t desired_top = expected_top;
                if(!top_.compare_exchange_weak(top, desired_top, std::memory_order_acquire))
                    return nullptr;
                return job;
            }
            return nullptr;
        }

        Job* Pop(){
            size_t bottom = bottom_.load(std::memory_order_acquire);
            bottom = std::max((size_t)0, bottom - 1);
            bottom_.store(bottom, std::memory_order_release);
            size_t top = top_.load(std::memory_order_acquire);

            if(top <= bottom){
                Job* job = jobs_[bottom];
                if(top != bottom){
                    return job;
                } else{
                    size_t expectedTop = top;
                    size_t desiredTop = top + 1;
                    if(!top_.compare_exchange_weak(expectedTop, desiredTop, std::memory_order_acq_rel))
                        job = nullptr;

                    bottom_.store(top + 1, std::memory_order_release);
                    return job;
                }
            } else{
                bottom_.store(top, std::memory_order_release);
                return nullptr;
            }
        }
    };
}

#endif //TOKEN_JOB_H
