#ifndef TOKEN_PEER_QUEUE_H
#define TOKEN_PEER_QUEUE_H

#include "address.h"
#include "work_stealing_queue.h"

namespace token{
#ifndef TOKEN_MAX_CONNECTION_ATTEMPTS
  #define TOKEN_MAX_CONNECTION_ATTEMPTS 3
#endif//TOKEN_MAX_CONNECTION_ATTEMPTS

#ifndef TOKEN_CONNECTION_QUEUE_SIZE
  #define TOKEN_CONNECTION_QUEUE_SIZE (FLAGS_num_peers * TOKEN_MAX_CONNECTION_ATTEMPTS)
#endif//TOKEN_CONNECTION_QUEUE_SIZE

#ifndef TOKEN_CONNECTION_BACKOFF_SCALE
  #ifdef TOKEN_DEBUG
    #define TOKEN_CONNECTION_BACKOFF_SCALE 5
  #else
    #define TOKEN_CONNECTION_BACKOFF_SCALE 15
  #endif//TOKEN_DEBUG
#endif//TOKEN_CONNECTION_BACKOFF_SCALE

  typedef int ConnectionAttemptCounter;

  class ConnectionRequest{
    friend class PeerSessionThread;
    friend class PeerSessionManager;
   public:
    static inline int
    CompareAttempts(const ConnectionRequest& a, const ConnectionRequest& b){
      if(a.attempts_ < b.attempts_){
        return -1;
      } else if(a.attempts_ > b.attempts_){
        return +1;
      }
      return 0;
    }

    static inline int
    Compare(const ConnectionRequest& a, const ConnectionRequest& b){
      int ret;
      if((ret = NodeAddress::Compare(a.address_, b.address_)) != 0)
        return ret; // not-equal
      return CompareAttempts(a, b);
    }
   private:
    NodeAddress address_;
    ConnectionAttemptCounter attempts_;

    ConnectionRequest(const NodeAddress& address, const ConnectionAttemptCounter& attempts=TOKEN_MAX_CONNECTION_ATTEMPTS):
      address_(address),
      attempts_(attempts){}
   public:
    ConnectionRequest():
      address_(),
      attempts_(0){}
    ConnectionRequest(const ConnectionRequest& other):
      address_(other.address_),
      attempts_(other.attempts_){}
    ~ConnectionRequest() = default;

    NodeAddress GetAddress() const{
      return address_;
    }

    ConnectionAttemptCounter GetNumberOfAttemptsRemaining() const{
      return attempts_;
    }

    bool CanReschedule() const{
      return (attempts_ - 1) >= 0;
    }

    void operator=(const ConnectionRequest& request){
      address_ = request.address_;
      attempts_ = request.attempts_;
    }

    friend bool operator==(const ConnectionRequest& a, const ConnectionRequest& b){
      return a.address_ == b.address_
          && a.attempts_ == b.attempts_;
    }

    friend bool operator!=(const ConnectionRequest& a, const ConnectionRequest& b){
      return !operator==(a, b);
    }

    friend bool operator<(const ConnectionRequest& a, const ConnectionRequest& b){
      if(a.address_ == b.address_)
        return a.attempts_ < b.attempts_;
      return a.address_ < b.address_;
    }
  };

  typedef WorkStealingQueue<ConnectionRequest*> ConnectionRequestQueue;
}

#endif//TOKEN_PEER_QUEUE_H