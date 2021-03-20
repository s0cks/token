#ifndef TOKEN_HTTP_CONTROLLER_POOL_H
#define TOKEN_HTTP_CONTROLLER_POOL_H

#include "pool.h"

#include <utility>
#include "http/http_controller.h"

namespace token{
#define FOR_EACH_POOL_CONTROLLER_ENDPOINT(V) \
  V(GET, "/pool/blocks/:hash", GetBlock)      \
  V(GET, "/pool/blocks", GetBlocks)          \
  V(GET, "/pool/transactions/:hash", GetTransaction) \
  V(GET, "/pool/transactions", GetTransactions)      \
  V(GET, "/pool/utxos/:hash", GetUnclaimedTransaction)     \
  V(GET, "/pool/utxos", GetUnclaimedTransactions)

  class PoolController : HttpController{
   protected:
    ObjectPoolPtr pool_;

#define DECLARE_ENDPOINT(Method, Path, Name) \
    HTTP_CONTROLLER_ENDPOINT(Name)

    FOR_EACH_POOL_CONTROLLER_ENDPOINT(DECLARE_ENDPOINT)
#undef DECLARE_ENDPOINT
   public:
    explicit PoolController(ObjectPoolPtr pool):
      HttpController(),
      pool_(std::move(pool)){}
    ~PoolController() override = default;

    ObjectPoolPtr GetPool() const{
      return pool_;
    }

    bool Initialize(HttpRouter* router) override{
#define REGISTER_ENDPOINT(Method, Path, Name) \
      HTTP_CONTROLLER_##Method(Path, Name);

      FOR_EACH_POOL_CONTROLLER_ENDPOINT(REGISTER_ENDPOINT)
#undef REGISTER_ENDPOINT
      return true;
    }
  };
}

#endif//TOKEN_HTTP_CONTROLLER_POOL_H