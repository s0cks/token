#ifndef TOKEN_DISCOVERY_H
#define TOKEN_DISCOVERY_H

#include "vthread.h"
#include "proposal.h"

namespace Token{
#define FOR_EACH_BLOCK_DISCOVERY_THREAD_STATE(V) \
    V(Starting)                                  \
    V(Running)                                   \
    V(Stopping)                                  \
    V(Stopped)

#define FOR_EACH_BLOCK_DISCOVERY_THREAD_STATUS(V) \
    V(Ok)                                         \
    V(Warning)                                    \
    V(Error)

  class BlockDiscoveryThread : public Thread{
   public:
    enum State{
#define DEFINE_STATE(Name) k##Name,
      FOR_EACH_BLOCK_DISCOVERY_THREAD_STATE(DEFINE_STATE)
#undef DEFINE_STATE
    };

    friend std::ostream& operator<<(std::ostream& stream, const State& state){
      switch(state){
#define DEFINE_TOSTRING(Name) \
                case State::k##Name: \
                    stream << #Name; \
                    return stream;
        FOR_EACH_BLOCK_DISCOVERY_THREAD_STATE(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
        default:stream << "Unknown";
          return stream;
      }
    }

    enum Status{
#define DEFINE_STATUS(Name) k##Name,
      FOR_EACH_BLOCK_DISCOVERY_THREAD_STATUS(DEFINE_STATUS)
#undef DEFINE_STATUS
    };

    friend std::ostream& operator<<(std::ostream& stream, const Status& status){
      switch(status){
#define DEFINE_TOSTRING(Name) \
                case Status::k##Name: \
                    stream << #Name; \
                    return stream;
        FOR_EACH_BLOCK_DISCOVERY_THREAD_STATUS(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
        default:stream << "Unknown";
          return stream;
      }
    }
   private:
    BlockDiscoveryThread() = delete;

    static void SetState(const State& state);
    static void SetStatus(const Status& status);
    static void HandleThread(uword parameter);
    static ProposalPtr CreateNewProposal(BlockPtr blk);
    static BlockPtr CreateNewBlock(intptr_t size);
   public:
    ~BlockDiscoveryThread() = delete;

    static State GetState();
    static Status GetStatus();
    static void WaitForState(const State& state);
    static void SetBlock(BlockPtr blk);
    static void SetProposal(const ProposalPtr& proposal);
    static BlockPtr GetBlock();
    static ProposalPtr GetProposal();
    static bool HasProposal();

    static inline void
    ClearProposal(){
      SetProposal(nullptr);
    }

    static bool Start();
    static bool Stop();

#define DEFINE_CHECK(Name) \
        static bool Is##Name(){ return GetState() == State::k##Name; }
    FOR_EACH_BLOCK_DISCOVERY_THREAD_STATE(DEFINE_CHECK)
#undef DEFINE_CHECK

#define DEFINE_CHECK(Name) \
        static bool Is##Name(){ return GetStatus() == Status::k##Name; }
    FOR_EACH_BLOCK_DISCOVERY_THREAD_STATUS(DEFINE_CHECK)
#undef DEFINE_CHECK
  };
}

#endif //TOKEN_DISCOVERY_H