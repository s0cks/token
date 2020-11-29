#ifndef TOKEN_TASK_H
#define TOKEN_TASK_H

#include "session.h"
#include "object.h"

namespace Token{
    class Task : public Object{
    protected:
        Task():
            Object(){
            SetType(Type::kTaskType);
        }
    public:
        ~Task() = default;
    };

    class SessionTask : public Task{
    protected:
        Session* session_;

        SessionTask(const Handle<Session>& session):
            Task(),
            session_(nullptr){
            WriteBarrier(&session_, session);
        }

#ifndef TOKEN_GCMODE_NONE
        bool Accept(WeakObjectPointerVisitor* vis){
            if(!vis->Visit(&session_)){
                LOG(WARNING) << "couldn't visit SessionTask's session.";
                return false;
            }

            return true;
        }
#endif//TOKEN_GCMODE_NONE
    public:
        ~SessionTask() = default;

        Handle<Session> GetSession() const{
            return session_;
        }
    };

#define DEFINE_ASYNC_TASK(Name) \
    public: \
        const char* GetName() const{ return #Name; }

    class HandleMessageTask : public SessionTask{
    private:
        Message* message_;

        HandleMessageTask(const Handle<Session>& session, const Handle<Message>& message):
            SessionTask(session),
            message_(nullptr){
            WriteBarrier(&message_, message);
        }

#ifndef TOKEN_GCMODE_NONE
    protected:
        bool Accept(WeakObjectPointerVisitor* vis){
            if(!SessionTask::Accept(vis)){
                return false;
            }

            if(!vis->Visit(&message_)){
                LOG(WARNING) << "couldn't visit HandleMessageTask's session.";
                return false;
            }

            return true;
        }
#endif//TOKEN_GCMODE_NONE
    public:
        ~HandleMessageTask() = default;

        Handle<Message> GetMessage() const{
            return message_;
        }

        static Handle<HandleMessageTask> NewInstance(const Handle<Session>& session, const Handle<Message>& msg){
            return new HandleMessageTask(session, msg);
        }
    };
}

#endif //TOKEN_TASK_H