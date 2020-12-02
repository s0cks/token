#ifndef TOKEN_BLOCK_QUEUE_H
#define TOKEN_BLOCK_QUEUE_H

#include "block.h"

namespace Token{
    class BlockQueue{
    public:
        static const size_t kMaxBlockQueueSize = 4;
    private:
        BlockQueue() = delete;
    public:
        ~BlockQueue() = delete;

        static void Queue(Block* blk);
        static Block* DeQueue();
        static size_t GetSize();
    };
}

#endif //TOKEN_BLOCK_QUEUE_H