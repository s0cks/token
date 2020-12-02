#include <deque>
#include <condition_variable>
#include "crash_report.h"
#include "block_queue.h"

namespace Token{
    static std::mutex mutex_;
    static std::condition_variable cond_;
    static std::deque<Block*> queue_;

    void BlockQueue::Queue(Block* blk){
        std::unique_lock<std::mutex> lock(mutex_);
        queue_.push_back(blk);
        lock.unlock();
        cond_.notify_one();
        if(GetSize() > BlockQueue::kMaxBlockQueueSize){
            LOG(WARNING) << "Block Queue is full (" << GetSize() << " Blocks)";
            return;
        }
    }

    Block* BlockQueue::DeQueue(){
        std::unique_lock<std::mutex> lock(mutex_);
        while(queue_.empty()) cond_.wait(lock);
        auto item = queue_.front();
        queue_.pop_front();
        return item;
    }

    size_t BlockQueue::GetSize(){
        std::unique_lock<std::mutex> lock(mutex_);
        return queue_.size();
    }
}