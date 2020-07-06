#ifndef TOKEN_TASK_H
#define TOKEN_TASK_H

#include "common.h"
#include "node/session.h"

namespace Token{
#define FOR_EACH_TASK(V) \
    V(Election)

#define FORWARD_DECLARE_TASK(Name) \
    class Name##Task;
    FOR_EACH_TASK(FORWARD_DECLARE_TASK);
#undef FORWARD_DECLARE_TASK

    class Task{
    public:
        virtual ~Task() = default;
    };

    class SessionTask : public Task{
    private:
        Session* session_;
    protected:
        SessionTask(Session* session):
            session_(session){}
    public:
        virtual ~SessionTask() = default;

        Session* GetSession() const{
            return session_;
        }
    };

    class HandleMessageTask : public SessionTask{
    private:
        Message* message_;
    public:
        HandleMessageTask(Session* session, Message* msg):
            SessionTask(session),
            message_(msg){}
        ~HandleMessageTask(){}

        Message* GetMessage() const{
            return message_;
        }
    };

    class InventoryTask : public SessionTask{
    protected:
        std::vector<InventoryItem> items_;

        InventoryTask(Session* session, std::vector<InventoryItem>& items):
            SessionTask(session),
            items_(items){}
    public:
        ~InventoryTask(){}

        bool GetItems(std::vector<InventoryItem>& items){
            for(auto& it : items_) items.push_back(it);
            return items.size() == items_.size();
        }
    };

    class SynchronizeBlocksTask : public InventoryTask{
    public:
        SynchronizeBlocksTask(Session* session, std::vector<InventoryItem>& items): InventoryTask(session, items){}
        ~SynchronizeBlocksTask(){}
    };
}

#endif //TOKEN_TASK_H