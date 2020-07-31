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
    };

#define DEFINE_ASYNC_TASK(Name) \
    public: \
        const char* GetName() const{ return #Name; }



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
}

#endif //TOKEN_TASK_H