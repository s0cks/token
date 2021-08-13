#ifndef TKN_RUNTIME_H
#define TKN_RUNTIME_H

#include <uv.h>

#include "pool.h"
#include "miner.h"
#include "config.h"
#include "eventbus.h"
#include "miner_listener.h"
#include "node/node_server.h"
#include "task/task_engine.h"
#include "consensus/acceptor.h"
#include "consensus/proposer.h"
#include "http/http_service_rest.h"
#include "http/http_service_health.h"
#include "consensus/proposal_timer.h"
#include "consensus/proposal_listener.h"

namespace token{
#define FOR_EACH_RUNTIME_STATE(V) \
  V(Starting)                     \
  V(Running)                      \
  V(Stopping)                     \
  V(Stopped)

  class Runtime : public TestEventListener{
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
    Configuration config_;
    EventBus events_;
    task::TaskQueue task_queue_;
    task::TaskEngine task_engine_;
    ObjectPool pool_;
    UUID node_id_;
    ProposalState proposal_state_;
    node::Server server_;
    http::RestService service_rest_;
    http::HealthService service_health_;
    BlockMiner miner_;
    ProposalTimer timer_;
    Proposer proposer_;
    Acceptor acceptor_;

    void SetState(const State& state){
      state_ = state;
    }
  protected:
    void HandleOnTestEvent() override{
      DLOG(INFO) << "test.";
    }
  public:
    explicit Runtime(uv_loop_t* loop=uv_loop_new());
    Runtime(const Runtime& rhs) = delete;
    ~Runtime() override = default;

    Configuration& GetConfig(){
      return config_;
    }

    UUID GetNodeId() const{
      return node_id_;
    }

    EventBus& GetEventBus(){
      return events_;
    }

    void Publish(const std::string& event){
      events_.Publish(event);
    }

    ProposalState& GetProposalState(){
      return proposal_state_;
    }

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

    ProposalTimer& GetProposalTimer(){
      return timer_;
    }

    node::Server& GetServer(){
      return server_;
    }

    http::RestService& GetRestService(){
      return service_rest_;
    }

    http::HealthService& GetHealthService(){
      return service_health_;
    }

    task::TaskQueue& GetTaskQueue(){
      return task_queue_;
    }

    task::TaskEngine& GetTaskEngine(){
      return task_engine_;
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