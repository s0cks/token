#ifndef TOKEN_HANDLE_MESSAGE_TASK_H
#define TOKEN_HANDLE_MESSAGE_TASK_H

#include "task.h"

namespace Token{
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

#endif //TOKEN_HANDLE_MESSAGE_TASK_H