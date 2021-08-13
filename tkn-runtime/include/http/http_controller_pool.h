#ifndef TOKEN_HTTP_CONTROLLER_POOL_H
#define TOKEN_HTTP_CONTROLLER_POOL_H

#include "pool.h"
#include "http/http_controller.h"

namespace token{
  namespace http{
#define FOR_EACH_POOL_CONTROLLER_ENDPOINT(V) \
  V(GET, "/pool/stats", GetPoolInfo)               \
  V(GET, "/pool/blocks/data/:hash", GetBlock)     \
  V(GET, "/pool/blocks", GetAllBlocks)          \
  V(GET, "/pool/data/transactions/:hash", GetTransaction) \
  V(GET, "/pool/transactions", GetTransactions)      \
  V(GET, "/pool/data/unclaimed_transactions/:hash", GetUnclaimedTransaction)     \
  V(GET, "/pool/unclaimed_transactions", GetUnclaimedTransactions)

    class PoolController : public Controller{
     protected:
      ObjectPool& pool_;
     public:
      explicit PoolController(ObjectPool& pool):
        Controller(),
        pool_(pool){}
      ~PoolController() override = default;

      ObjectPool& GetPool(){
        return pool_;
      }

      DECLARE_HTTP_CONTROLLER(PoolController, FOR_EACH_POOL_CONTROLLER_ENDPOINT);
    };
  }
}

#endif//TOKEN_HTTP_CONTROLLER_POOL_H