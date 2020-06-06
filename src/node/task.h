#ifndef TOKEN_TASK_H
#define TOKEN_TASK_H

#include "common.h"

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
        ~HandleMessageTask(){
            if(message_) delete message_;
        }

        Message* GetMessage() const{
            return message_;
        }
    };

    class GetDataTask : public SessionTask{
    private:
        std::vector<InventoryItem> items_;
    public:
        GetDataTask(Session* session, std::vector<InventoryItem>& items):
            SessionTask(session),
            items_(items){}
        ~GetDataTask(){}

        bool GetItems(std::vector<InventoryItem>& items){
            for(auto& it : items_) items.push_back(it);
            return items.size() == items_.size();
        }

        bool HasMoreItems(){
            return items_.size() > 0;
        }

        uint32_t GetItemCount(){
            return items_.size();
        }

        InventoryItem GetNextItem(){
            InventoryItem item = items_.back();
            items_.pop_back();
            return item;
        }
    };
}

#endif //TOKEN_TASK_H