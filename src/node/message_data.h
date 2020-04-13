#ifndef TOKEN_REQUEST_H
#define TOKEN_REQUEST_H

#include <uv.h>
#include "node/session.h"
#include "node/message.h"

namespace Token{
    struct ResolveInventoryData{
        Session* session;
        std::vector<std::string> hashes;

        ResolveInventoryData(Session* sess, const std::vector<std::string>& hash_list):
            session(sess),
            hashes(hash_list){}
    };

    struct BroadcastBlockData{
        Block* block;

        BroadcastBlockData(Block* blk):
            block(blk){}
    };

    struct ProcessMessageData{
        Session* session;
        Message* request;

        ProcessMessageData(Session* sess, Message* msg):
            session(sess),
            request(msg){}
    };
}

#endif //TOKEN_REQUEST_H