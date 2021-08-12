#ifndef TKN_EVENTBUS_H
#define TKN_EVENTBUS_H

#include <uv.h>
#include <string>
#include <vector>

#include "common.h"
#include "trie.h"

namespace token{
#ifdef TOKEN_DEBUG
  class TestEventListener{
  protected:
    uv_async_t on_test_;

    explicit TestEventListener(uv_loop_t* loop):
      on_test_(){
      on_test_.data = this;
      CHECK_UVRESULT2(FATAL, uv_async_init(loop, &on_test_, [](uv_async_t* handle){
        ((TestEventListener*)handle->data)->HandleOnTestEvent();
      }), "cannot initialize the on_test_ callback handle");
    }

    virtual void HandleOnTestEvent() = 0;
  public:
    virtual ~TestEventListener() = default;
  };
#endif//TOKEN_DEBUG

  class EventBus{
  protected:
    class EventBusHandle{
    private:
      uv_async_t* handle_;
    public:
      explicit EventBusHandle(uv_async_t* handle):
        handle_(handle){}
      EventBusHandle(const EventBusHandle& rhs) = default;
      ~EventBusHandle() = default;

      bool Invoke(){
        int err;
        if((err = uv_async_send(handle_)) != 0){
          DLOG(ERROR) << "cannot invoke EventBusHandler handle_: " << uv_strerror(err);
          return false;
        }
        return true;
      }

      EventBusHandle& operator=(const EventBusHandle& rhs) = default;
    };

    typedef std::vector<EventBusHandle> EventBusHandleList;

    class EventBusHandles{
      friend class EventBus;
    private:
      EventBusHandleList handles_;
    public:
      EventBusHandles():
        handles_(){}
      EventBusHandles(const std::initializer_list<EventBusHandle>& handles):
        handles_(handles){}
      EventBusHandles(const EventBusHandles& rhs) = default;
      ~EventBusHandles() = default;

      void AddHandle(const EventBusHandle& handle){
        handles_.push_back(handle);
      }

      EventBusHandles& operator=(const EventBusHandles& rhs) = default;
    };

    Trie<std::string, EventBusHandles, 128> trie_;
  public:
    EventBus():
      trie_(){}
    ~EventBus() = default;

    bool Publish(const std::string& name){
      auto node = trie_.Search(name);
      if(!node){
        DLOG(WARNING) << "cannot find event handles for " << name << ".";
        return false;
      }

      for(auto& handler : node->handles_){
        if(!handler.Invoke()){
          DLOG(ERROR) << "cannot invoke callback handler for event '" << name << "'";
          return false;
        }
      }
      return true;
    }

    void Subscribe(const std::string& name, uv_async_t* handle){
      EventBusHandles* route = nullptr;
      if(trie_.Contains(name)){
        route = trie_.Search(name);
      } else{
        route = new EventBusHandles();
        trie_.Insert(name, route);
      }
      route->AddHandle(EventBusHandle(handle));
    }
  };
}

#endif//TKN_EVENTBUS_H