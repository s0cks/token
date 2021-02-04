#ifndef TOKEN_INSPECTOR_H
#define TOKEN_INSPECTOR_H

#include <uv.h>
#include <deque>
#include <string>
#include <glog/logging.h>
#include "common.h"

namespace token{
  template<typename DataType, typename CommandType>
  class Inspector{
   protected:
    uv_pipe_t stdin_;
    uv_pipe_t stdout_;
    DataType* data_;

    Inspector():
      stdin_(),
      stdout_(),
      data_(nullptr){
      stdin_.data = this;
      stdout_.data = this;
    }

    virtual void OnCommand(CommandType* cmd) = 0;

    inline void
    SetData(DataType* data){
      if(data_) delete data_;
      data_ = data;
    }

    static void AllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buff){
      buff->base = (char*) malloc(4096);
      buff->len = 4096;
    }

    static void OnCommandReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff){
      Inspector* inspector = (Inspector*) stream->data;
      if(nread == UV_EOF){
        LOG(ERROR) << "client disconnected!";
        return;
      } else if(nread < 0){
        LOG(ERROR) << "[" << nread << "] client read error: " << std::string(uv_strerror(nread));
        return;
      } else if(nread == 0){
        LOG(WARNING) << "zero message size received";
        return;
      } else if(nread >= 4096){
        LOG(ERROR) << "too large of a buffer";
        return;
      }

      std::string line(buff->base, nread - 1);
      std::deque<std::string> args;
      SplitString(line, args, ' ');

      CommandType cmd(args);
      inspector->OnCommand(&cmd);
    }
   public:
    virtual ~Inspector() = default;

    DataType* GetData() const{
      return data_;
    }

    bool HasData() const{
      return data_ != nullptr;
    }

    void Inspect(DataType* data){
      SetData(data);
      uv_loop_t* loop = uv_loop_new();
      uv_loop_init(loop);
      uv_pipe_init(loop, &stdin_, 0);
      uv_pipe_open(&stdin_, 0);
      uv_pipe_init(loop, &stdout_, 0);
      uv_pipe_open(&stdout_, 1);

      int err;
      if((err = uv_read_start((uv_stream_t*) &stdin_, &AllocBuffer, &OnCommandReceived)) != 0){
        LOG(WARNING) << "couldn't start reading from stdin-pipe: " << uv_strerror(err);
        return; //TODO: shutdown properly
      }

      uv_run(loop, UV_RUN_DEFAULT);
      uv_read_stop((uv_stream_t*) &stdin_);
      uv_loop_close(loop);
      SetData(nullptr);
    }
  };
}

#endif //TOKEN_INSPECTOR_H