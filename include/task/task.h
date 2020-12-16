#ifndef TOKEN_TASK_H
#define TOKEN_TASK_H

#include "session.h"
#include "object.h"

namespace Token{
    class Task : public Object{
    protected:
        Task() = default;
    public:
        ~Task() = default;
    };

    class SessionTask : public Task{
    protected:
        Session* session_;

        SessionTask(Session* session):
            Task(),
            session_(session){}
    public:
        ~SessionTask() = default;

        Session* GetSession() const{
            return session_;
        }

        template<typename T>
        T* GetSession() const{
            return (T*)session_;
        }
    };

#define DEFINE_ASYNC_TASK(Name) \
    public: \
        const char* GetName() const{ return #Name; }

    class AsyncTaskResult{
    public:
        enum Status{
            kSuccessful,
            kFailed,
            kCancelled,
            kTimedOut
        };

        friend std::ostream& operator<<(std::ostream& stream, const Status& status){
            switch(status){
                case kFailed:
                    stream << "Failed";
                    return stream;
                case kSuccessful:
                    stream << "Success";
                    return stream;
                case kCancelled:
                    stream << "Cancelled";
                    return stream;
                case kTimedOut:
                    stream << "Timed Out";
                    return stream;
                default:
                    stream << "Unknown";
                    return stream;
            }
        }
    private:
        Status status_;
        std::string message_;
    public:
        AsyncTaskResult(Status status, const std::string& msg):
            status_(status),
            message_(msg){}
        AsyncTaskResult(Status status, const std::stringstream& ss):
            AsyncTaskResult(status, ss.str()){}
        AsyncTaskResult(const AsyncTaskResult& other):
            status_(other.status_),
            message_(other.message_){}
        ~AsyncTaskResult() = default;

        Status GetStatus() const{
            return status_;
        }

        std::string GetMessage() const{
            return message_;
        }

        void operator=(const AsyncTaskResult& other){
            status_ = other.status_;
            message_ = other.message_;
        }

        friend std::ostream& operator<<(std::ostream& stream, const AsyncTaskResult& result){
            stream << "[" << result.status_ << "] " << result.message_;
            return stream;
        }

        static inline AsyncTaskResult
        Success(const std::string& msg){
            return AsyncTaskResult(AsyncTaskResult::kSuccessful, msg);
        }

        static inline AsyncTaskResult
        Failed(const std::string& msg){
            return AsyncTaskResult(AsyncTaskResult::kFailed, msg);
        }

        static inline AsyncTaskResult
        Cancelled(const std::string& msg){
            return AsyncTaskResult(AsyncTaskResult::kCancelled, msg);
        }
    };

    class AsyncTask : public Task{
        //TOOD: merge w/ Job
    private:
        void SetResult(const AsyncTaskResult& result){
            result_ = result;
        }
    protected:
        uv_loop_t* loop_;
        uv_work_t handle_;
        AsyncTaskResult result_;

        AsyncTask(uv_loop_t* loop):
            loop_(loop),
            handle_(),
            result_(AsyncTaskResult::kTimedOut, "no execution?"){
            handle_.data = this;
        }

        virtual AsyncTaskResult DoWork() = 0;

        static void OnTask(uv_work_t* handle){
            AsyncTask* task = (AsyncTask*)handle->data;
            task->SetResult(task->DoWork());
        }

        static void OnTaskFinished(uv_work_t* handle, int status){
            AsyncTask* task = (AsyncTask*)handle->data;
            AsyncTaskResult result = task->GetResult();
            switch(result.GetStatus()){
                case AsyncTaskResult::kSuccessful:
                    LOG(INFO) << task->GetName() << " finished: " << task->GetResult();
                    return;
                case AsyncTaskResult::kFailed:
                    LOG(INFO) << task->GetName() << " failed: " << task->GetResult();
                    return;
                case AsyncTaskResult::kCancelled:
                    LOG(WARNING) << task->GetName() << " was cancelled: " << task->GetResult();
                    return;
                case AsyncTaskResult::kTimedOut:
                    LOG(WARNING) << task->GetName() << " timed out: " << task->GetResult();
                    return;
            }
        }
    public:
        virtual ~AsyncTask() = default;
        virtual const char* GetName() const = 0;

        AsyncTaskResult GetResult() const{
            return result_;
        }

        bool Submit(){
            return uv_queue_work(loop_, &handle_, &OnTask, &OnTaskFinished) == 0;
        }

        bool Cancel(){
            SetResult(AsyncTaskResult::Cancelled("Task was cancelled"));
            return uv_cancel((uv_req_t*)&handle_) == 0;
        }
    };

    class AsyncSessionTask : public AsyncTask{
    protected:
        Session* session_;

        AsyncSessionTask(uv_loop_t* loop, Session* session):
            AsyncTask(loop),
            session_(session){}
    public:
        virtual ~AsyncSessionTask() = default;

        Session* GetSession() const{
            return session_;
        }
    };
}

#endif //TOKEN_TASK_H