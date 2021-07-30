#ifndef TKN_RUNTIME_H
#define TKN_RUNTIME_H

#include <uv.h>

#include "pool.h"
#include "miner.h"
#include "acceptor.h"
#include "proposer.h"
#include "node/node_server.h"
#include "task/task_engine.h"

namespace token{
#define FOR_EACH_RUNTIME_STATE(V) \
  V(Starting)                     \
  V(Running)                      \
  V(Stopping)                     \
  V(Stopped)

  class Runtime{
  public:
    enum State{
#define DEFINE_STATE(Name) k##Name,
      FOR_EACH_RUNTIME_STATE(DEFINE_STATE)
#undef DEFINE_STATE
    };

    inline friend std::ostream&
    operator<<(std::ostream& stream, const State& state){
      switch(state){
#define DEFINE_TOSTRING(Name) \
        case State::k##Name:  \
          return stream << #Name;
        FOR_EACH_RUNTIME_STATE(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
        default:
          return stream << "Unknown";
      }
    }
  private:
    uv_loop_t* loop_;
    atomic::RelaxedAtomic<State> state_;

    task::TaskQueue task_queue_;
    task::TaskEngine task_engine_;

    ObjectPool pool_;

    node::Server server_;

    BlockMiner miner_;
    Proposer proposer_;
    Acceptor acceptor_;

    void SetState(const State& state){
      state_ = state;
    }
  public:
    explicit Runtime(uv_loop_t* loop=uv_loop_new());
    Runtime(const Runtime& rhs) = delete;
    ~Runtime() = default;

    uv_loop_t* loop() const{
      return loop_;
    }

    State GetState() const{
      return (State)state_;
    }

    ObjectPool& GetPool(){
      return pool_;
    }

    Proposer& GetProposer(){
      return proposer_;
    }

    Acceptor& GetAcceptor(){
      return acceptor_;
    }

    BlockMiner& GetBlockMiner(){
      return miner_;
    }

    node::Server& GetServer(){
      return server_;
    }

    task::TaskQueue& GetTaskQueue(){
      return task_queue_;
    }

    task::TaskEngine& GetTaskEngine(){
      return task_engine_;
    }

    bool StartProposal(){
      return GetProposer().StartProposal();
    }

    bool Run();

#define DEFINE_STATE_CHECK(Name) \
    inline bool Is##Name() const{ return GetState() == State::k##Name; }
    FOR_EACH_RUNTIME_STATE(DEFINE_STATE_CHECK)
#undef DEFINE_STATE_CHECK

    Runtime& operator=(const Runtime& rhs) = delete;
    friend bool operator==(const Runtime& lhs, const Runtime& rhs) = delete;
    friend bool operator!=(const Runtime& lhs, const Runtime& rhs) = delete;
    friend bool operator<(const Runtime& lhs, const Runtime& rhs) = delete;
    friend bool operator>(const Runtime& lhs, const Runtime& rhs) = delete;
  };
}

#endif//TKN_RUNTIME_H