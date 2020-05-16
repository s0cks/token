#ifndef TOKEN_REQUEST_H
#define TOKEN_REQUEST_H

#include <uv.h>
#include "node/session.h"
#include "node/message.h"

namespace Token{
    struct ProcessMessageData{
        Session* session;
        Message* request;

        ProcessMessageData(Session* sess, Message* msg):
            session(sess),
            request(msg){}
    };

    struct BroadcastInventoryData{
        InventoryMessage* inventory;

        BroadcastInventoryData(InventoryMessage* inv):
            inventory(inv){}
    };

    struct ResolveInventoryData{
        Session* session;
        std::vector<InventoryItem> items;

        ResolveInventoryData(Session* sess, std::vector<InventoryItem>& itms):
            session(sess),
            items(itms){}
    };
}

#endif //TOKEN_REQUEST_H