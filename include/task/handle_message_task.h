#ifndef TOKEN_HANDLE_MESSAGE_TASK_H
#define TOKEN_HANDLE_MESSAGE_TASK_H

#include "task.h"

namespace Token{
  class HandleMessageTask : public SessionTask{
   private:
    MessagePtr message_;

    HandleMessageTask(Session* session, const MessagePtr& msg):
      SessionTask(session),
      message_(msg){}
   public:
    ~HandleMessageTask() = default;

    MessagePtr GetMessage() const{
      return message_;
    }

    template<typename T>
    std::shared_ptr<T> GetMessage() const{
      return std::static_pointer_cast<T>(message_);
    }

    std::string ToString() const{
      //TODO: refactor
      std::stringstream ss;
      ss << "HandleMessageTask(" << GetMessage()->ToString() << ")";
      return ss.str();
    }

    static HandleMessageTask* NewInstance(Session* session, const MessagePtr& msg){
      return new HandleMessageTask(session, msg);
    }
  };
}

#endif //TOKEN_HANDLE_MESSAGE_TASK_H