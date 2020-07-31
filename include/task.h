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

    class AsyncTask : public Task{
    protected:
        uv_loop_t* loop_;
        uv_work_t handle_;

        AsyncTask(uv_loop_t* loop):
            loop_(loop),
            handle_(){
            handle_.data = this;
        }

        virtual bool DoWork(){ return true; }

        static void OnTask(uv_work_t* handle){
            AsyncTask* task = (AsyncTask*)handle->data;
            if(!task->DoWork()){
                //TODO: error
            }
        }

        static void OnTaskFinished(uv_work_t* handle, int status){
            AsyncTask* task = (AsyncTask*)handle->data;

        }
    public:
        virtual ~AsyncTask() = default;

        bool Submit(){
            return uv_queue_work(loop_, &handle_, &OnTask, &OnTaskFinished) == 0;
        }

        bool Cancel(){
            return uv_cancel((uv_req_t*)&handle_) == 0;
        }
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
    };

    class AsyncSessionTask : public AsyncTask{
    protected:
        Session* session_;

        AsyncSessionTask(uv_loop_t* loop, Session* session):
            AsyncTask(loop),
            session_(session){}

        virtual bool DoWork(){ return true; }
    public:
        virtual ~AsyncSessionTask() = default;

        Session* GetSession() const{
            return session_;
        }
    };

    class HandleMessageTask : public SessionTask{
    private:
        Message* message_;

        HandleMessageTask(Session* session, const Handle<Message>& message):
            SessionTask(session),
            message_(){
            WriteBarrier(&message_, message);
        }
    protected:
        virtual void Accept(WeakReferenceVisitor* vis){
            vis->Visit(&message_);
        }
    public:
        ~HandleMessageTask() = default;

        Handle<Message> GetMessage() const{
            return message_;
        }

        static Handle<HandleMessageTask> NewInstance(Session* session, const Handle<Message>& msg){
            return new HandleMessageTask(session, msg);
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
        bool DoWork();
    public:
        ~SynchronizeBlockChainTask() = default;

        InventoryItem GetHead() const{
            return InventoryItem(InventoryItem::kBlock, head_);
        }

        static Handle<SynchronizeBlockChainTask> NewInstance(uv_loop_t* loop, Session* session, const uint256_t& head){
            return new SynchronizeBlockChainTask(loop, session, head);
        }
    };
}

#endif //TOKEN_TASK_H