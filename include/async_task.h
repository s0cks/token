#ifndef TOKEN_ASYNC_TASK_H
#define TOKEN_ASYNC_TASK_H

#include <uv.h>
#include "task.h"
#include "snapshot.h"
#include "proposal.h"

namespace Token{
    class Result{
    public:
        enum Status{
            kSuccessful,
            kFailed,
            kCancelled,
            kTimedOut
        };
    private:
        Status status_;
        std::string message_;

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
    public:
        Result(Status status, const std::string& msg):
                status_(status),
                message_(msg){}
        Result(Status status, const std::stringstream& ss): Result(status, ss.str()){}
        Result(const Result& other):
                status_(other.status_),
                message_(other.message_){}
        ~Result() = default;

        Status GetStatus() const{
            return status_;
        }

        std::string GetMessage() const{
            return message_;
        }

        void operator=(const Result& other){
            status_ = other.status_;
            message_ = other.message_;
        }

        friend std::ostream& operator<<(std::ostream& stream, const Result& result){
            stream << "[" << result.status_ << "] " << result.message_;
            return stream;
        }

        static inline Result
        Success(const std::string& msg){
            return Result(Result::kSuccessful, msg);
        }

        static inline Result
        Failed(const std::string& msg){
            return Result(Result::kFailed, msg);
        }
    };

    class AsyncTask : public Task{
    private:
        void SetResult(const Result& result){
            result_ = result;
        }
    protected:
        uv_loop_t* loop_;
        uv_work_t handle_;
        Result result_;

        AsyncTask(uv_loop_t* loop):
            loop_(loop),
            handle_(),
            result_(Result::kFailed, "Task did not execute"){
            handle_.data = this;
        }

        virtual Result DoWork() = 0;

        static void OnTask(uv_work_t* handle){
            AsyncTask* task = (AsyncTask*)handle->data;
            task->SetResult(task->DoWork());
        }

        static void OnTaskFinished(uv_work_t* handle, int status){
            AsyncTask* task = (AsyncTask*)handle->data;
            Result result = task->GetResult();
            switch(result.GetStatus()){
                case Result::kSuccessful:
                    LOG(INFO) << task->GetName() << " finished: " << task->GetResult();
                    return;
                case Result::kFailed:
                    LOG(INFO) << task->GetName() << " failed: " << task->GetResult();
                    return;
                case Result::kCancelled:
                    LOG(WARNING) << task->GetName() << " was cancelled: " << task->GetResult();
                    return;
                case Result::kTimedOut:
                    LOG(WARNING) << task->GetName() << " timed out: " << task->GetResult();
                    return;
            }
        }
    public:
        virtual ~AsyncTask() = default;
        virtual const char* GetName() const = 0;

        Result GetResult() const{
            return result_;
        }

        bool Submit(){
            return uv_queue_work(loop_, &handle_, &OnTask, &OnTaskFinished) == 0;
        }

        bool Cancel(){
            SetResult(Result(Result::kCancelled, "Task was cancelled"));
            return uv_cancel((uv_req_t*)&handle_) == 0;
        }
    };

    class AsyncTaskThread : public Thread{
    private:
        AsyncTaskThread() = delete;

        static void SetState(Thread::State state);
        static void SetLoop(uv_loop_t* loop);
        static void HandleThread(uword parameter);
    public:
        ~AsyncTaskThread() = delete;

        static Thread::State GetState();
        static void WaitForState(Thread::State state);
        static uv_loop_t* GetLoop();

        static inline bool
        IsRunning(){
            return GetState() == Thread::State::kRunning;
        }

        static inline bool
        IsPaused(){
            return GetState() == Thread::State::kPaused;
        }

        static inline bool
        IsStopped(){
            return GetState() == Thread::State::kStopped;
        }

        static bool Start(){
            //TODO: fix parameter
            return Thread::Start("AsyncTaskThread", &HandleThread, 0);
        }

        static bool Pause(){
            if(!IsRunning()) return false;
            LOG(INFO) << "pausing the async task thread....";
            SetState(Thread::kPaused);
            return true;
        }

        static bool Stop(){
            //TODO: implement AsyncTaskThread::Stop
            LOG(WARNING) << "cannot stop AsyncTaskThread";
            return false;
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

    class SynchronizeBlockChainTask : public AsyncSessionTask{
    private:
        BlockHeader head_;

        SynchronizeBlockChainTask(uv_loop_t* loop, Session* session, const BlockHeader& head):
            AsyncSessionTask(loop, session),
            head_(head){}

        static inline InventoryItem
        GetItem(const Hash hash){
            return InventoryItem(InventoryItem::kBlock, hash);
        }

        bool ProcessBlock(const Handle<Block>& block);
        Result DoWork();
    public:
        ~SynchronizeBlockChainTask() = default;

        DEFINE_ASYNC_TASK(SynchronizeBlockChain);

        InventoryItem GetHead() const{
            return InventoryItem(InventoryItem::kBlock, head_.GetHash());
        }

        static Handle<SynchronizeBlockChainTask> NewInstance(uv_loop_t* loop, Session* session, const BlockHeader& head){
            return new SynchronizeBlockChainTask(loop, session, head);
        }
    };

    class SnapshotTask : public AsyncTask{
    protected:
        SnapshotTask(uv_loop_t* loop):
            AsyncTask(loop){}

        Result DoWork(){
            LOG(INFO) << "generating new snapshot....";
            if(!Snapshot::WriteNewSnapshot()){
                LOG(WARNING) << "couldn't write new snapshot!";
                return Result::Failed("Couldn't write new snapshot!");
            }
            return Result::Success("Snapshot Written!");
        }
    public:
        ~SnapshotTask() = default;

        DEFINE_ASYNC_TASK(Snapshot);

        static Handle<SnapshotTask> NewInstance(uv_loop_t* loop=AsyncTaskThread::GetLoop()){
            return new SnapshotTask(loop);
        }

        static bool ScheduleNewTask(uv_loop_t* loop=AsyncTaskThread::GetLoop()){
            return NewInstance(loop)->Submit();
        }
    };
}

#endif //TOKEN_ASYNC_TASK_H