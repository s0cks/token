#ifndef TOKEN_BLOCK_QUEUE_H
#define TOKEN_BLOCK_QUEUE_H

#include <pthread.h>
#include "block.h"

namespace Token{
    class BlockQueue{
    private:
        Block** data_;
        uint32_t max_;
        uint32_t front_;
        uint32_t back_;
        pthread_cond_t cond_;
        pthread_mutex_t mutex_;
    public:
        BlockQueue(uint32_t size);
        ~BlockQueue();

        bool Empty();
        bool Push(Block* block);
        bool Pop(Block** result);
    };
}

#endif //TOKEN_BLOCK_QUEUE_H
