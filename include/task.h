#ifndef TOKEN_TASK_H
#define TOKEN_TASK_H

#include "session.h"
#include "object.h"

namespace Token{
    class Task : public Object{
    protected:
        Task():
            Object(Type::kTaskType){}
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

    class HandleMessageTask : public SessionTask{
    private:
        Message* message_;

        HandleMessageTask(Session* session, Message* msg):
            SessionTask(session),
            message_(msg){}
    public:
        ~HandleMessageTask() = default;

        Message* GetMessage() const{
            return message_;
        }

        template<typename T>
        T* GetMessage() const{
            return (T*)message_;
        }

        static HandleMessageTask* NewInstance(Session* session, Message* msg){
            return new HandleMessageTask(session, msg);
        }
    };
}

#endif //TOKEN_TASK_H