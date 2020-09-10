#ifndef TOKEN_TASK_H
#define TOKEN_TASK_H

#include "session.h"
#include "object.h"

namespace Token{
    class Task : public Object{
    protected:
        Task() = default;

        size_t GetBufferSize() const{ return 0; }
        bool Encode(uint8_t* bytes) const{ return false; }
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
        bool Accept(WeakReferenceVisitor* vis){
            return vis->Visit(&message_);
        }
    public:
        ~HandleMessageTask() = default;

        Handle<Message> GetMessage() const{
            return message_;
        }

        size_t GetBufferSize() const{ return 0; } //TODO: implement
        bool Encode(ByteBuffer* bytes) const{ return false; } // TODO: implement

        static Handle<HandleMessageTask> NewInstance(Session* session, const Handle<Message>& msg){
            return new HandleMessageTask(session, msg);
        }
    };
}

#endif //TOKEN_TASK_H