#ifndef TOKEN_CLIENT_H
#define TOKEN_CLIENT_H

#include <pthread.h>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <queue>
#include "bytes.h"
#include "message.h"

namespace Token{
    class Client{
    private:
        template<typename Type>
        class WorkQueue{
        private:
            std::queue<Type> queue_;
            std::mutex mutex_;
            std::condition_variable cv_;
        public:
            WorkQueue():
                queue_(),
                mutex_(),
                cv_(){

            }
            ~WorkQueue(){}

            void Enqueue(Type& value){
                std::lock_guard<std::mutex> lock(mutex_);
                queue_.push(value);
                cv_.notify_one();
            }

            Type& Dequeue(){
                std::unique_lock<std::mutex> lock(mutex_);
                cv_.wait(lock, [&]{
                    return !queue_.empty();
                });
                Type& value = queue_.front();
                queue_.pop();
                return value;
            }
        };

        int socket_;
        std::string address_;
        int port_;
        pthread_t thread_id_;
        ByteBuffer read_buffer_;
        WorkQueue<Message*> queue_;

        static void* ClientThread(void* args);
    public:
        Client():
            queue_(),
            socket_(-1),
            read_buffer_(4096){
        }
        ~Client(){

        }

        void Connect(const std::string& addr, int port);
    };
}

#endif //TOKEN_CLIENT_H