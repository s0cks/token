#ifndef TKN_ACCEPTOR_H
#define TKN_ACCEPTOR_H

namespace token{
  class Runtime;
  class Acceptor{
  private:
    Runtime* runtime_;
    uv_async_t on_start_;

    static void HandleOnStart(uv_async_t* handle);
  public:
    explicit Acceptor(Runtime* runtime);
    ~Acceptor() = default;

    Runtime* GetRuntime() const{
      return runtime_;
    }
  };
}

#endif//TKN_ACCEPTOR_H