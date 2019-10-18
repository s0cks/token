#include <glog/logging.h>
#include "block_queue.h"

namespace Token{
    BlockQueue::BlockQueue(uint32_t size):
        data_(nullptr),
        max_(size),
        front_(size - 1),
        back_(size - 1),
        mutex_(PTHREAD_MUTEX_INITIALIZER),
        cond_(PTHREAD_COND_INITIALIZER){
        pthread_cond_init(&cond_, NULL);
        pthread_mutex_init(&mutex_, NULL);
        data_ = (Block**)malloc(sizeof(Block*) * size);
    }

    BlockQueue::~BlockQueue(){
        free(data_);
    }

    bool BlockQueue::Empty(){
        return front_ == back_;
    }

    bool BlockQueue::Push(Token::Block* block){
        pthread_mutex_lock(&mutex_);
        if(back_ == max_ - 1){
            back_ = 0;
        } else{
            back_++;
        }

        if(Empty()){
            if(back_ == 0){
                back_ = max_ - 1;
            } else{
                back_--;
            }

            pthread_mutex_unlock(&mutex_);
            return false;
        }

        LOG(INFO) << "Adding new block: " << block->GetHash();
        data_[back_] = block;
        pthread_cond_broadcast(&cond_);
        pthread_mutex_unlock(&mutex_);
        return true;
    }

    bool BlockQueue::Pop(Token::Block** result){
        if(!Empty()) return false;
        pthread_mutex_lock(&mutex_);
        pthread_cond_wait(&cond_, &mutex_);

        if(front_ == max_ - 1){
            front_ = 0;
        } else{
            front_++;
        }

        *result = data_[front_];
        pthread_mutex_unlock(&mutex_);
        return true;
    }
}