#include "runtime.h"
#include "acceptor.h"

namespace token{
  Acceptor::Acceptor(Runtime* runtime):
    runtime_(runtime){
    on_start_.data = this;
    CHECK_UVRESULT2(FATAL, uv_async_init(runtime->loop(), &on_start_, &HandleOnStart), "cannot initialize the on_start_ callbacck");
  }

  void Acceptor::HandleOnStart(uv_async_t* handle){
    auto acceptor = (Acceptor*)handle->data;
    DLOG(INFO) << "accepting proposal....";
  }
}