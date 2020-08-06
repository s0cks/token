#ifndef TOKEN_ASYNC_TASK_H
#define TOKEN_ASYNC_TASK_H

#include <uv.h>
#include "task.h"
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
                result_(Result::kFailed, "Task did not execute"),
                loop_(loop),
                handle_(){
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

    class HandleProposalTask : public AsyncTask{
    private:
        Proposal* proposal_;
        Block* block_;

        Result DoWork();
    protected:
        void Accept(WeakReferenceVisitor* vis){
            vis->Visit(&proposal_);
            vis->Visit(&block_);
        }
    public:
        HandleProposalTask(uv_loop_t* loop, const Handle<Proposal>& proposal, const Handle<Block>& blk):
            AsyncTask(loop),
            proposal_(nullptr),
            block_(nullptr){
            WriteBarrier(&proposal_, proposal);
            WriteBarrier(&block_, blk);
        }
        ~HandleProposalTask() = default;

        DEFINE_ASYNC_TASK(HandleProposal);

        Handle<Block> GetBlock() const{
            return block_;
        }

        Handle<Proposal> GetProposal() const{
            return proposal_;
        }

        static Handle<HandleProposalTask> NewInstance(uv_loop_t* loop, const Handle<Proposal>& proposal, const Handle<Block>& blk){
            return new HandleProposalTask(loop, proposal, blk);
        }
    };

    class SynchronizeBlockChainTask : public AsyncSessionTask{
    private:
        uint256_t head_;

        SynchronizeBlockChainTask(uv_loop_t* loop, Session* session, const uint256_t& head):
            AsyncSessionTask(loop, session),
            head_(head){}

        static inline InventoryItem
        GetItem(const uint256_t hash){
            return InventoryItem(InventoryItem::kBlock, hash);
        }

        bool ProcessBlock(const Handle<Block>& block);
        Result DoWork();
    public:
        ~SynchronizeBlockChainTask() = default;

        DEFINE_ASYNC_TASK(SynchronizeBlockChain);

        InventoryItem GetHead() const{
            return InventoryItem(InventoryItem::kBlock, head_);
        }

        static Handle<SynchronizeBlockChainTask> NewInstance(uv_loop_t* loop, Session* session, const uint256_t& head){
            return new SynchronizeBlockChainTask(loop, session, head);
        }
    };
}

#endif //TOKEN_ASYNC_TASK_H