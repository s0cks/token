#ifndef TOKEN_HTTP_CONTROLLER_POOL_H
#define TOKEN_HTTP_CONTROLLER_POOL_H

#include "pool.h"
#include "http/http_controller.h"

namespace token{
  namespace http{
    class PoolController;
    typedef std::shared_ptr<PoolController> PoolControllerPtr;

#define FOR_EACH_POOL_CONTROLLER_ENDPOINT(V) \
  V(GET, "/pool/stats", GetPoolInfo)               \
  V(GET, "/pool/data/blocks/:hash", GetBlock)     \
  V(GET, "/pool/blocks", GetAllBlocks)          \
  V(GET, "/pool/data/transactions/:hash", GetTransaction) \
  V(GET, "/pool/transactions", GetTransactions)      \
  V(GET, "/pool/data/unclaimed_transactions/:hash", GetUnclaimedTransaction)     \
  V(GET, "/pool/unclaimed_transactions", GetUnclaimedTransactions)

    class PoolController : public Controller,
                           public std::enable_shared_from_this<PoolController>{
     protected:
      ObjectPool& pool_;

#define DECLARE_ENDPOINT(Method, Path, Name) \
    HTTP_CONTROLLER_ENDPOINT(Name)

      FOR_EACH_POOL_CONTROLLER_ENDPOINT(DECLARE_ENDPOINT)
#undef DECLARE_ENDPOINT
     public:
      explicit PoolController(ObjectPool& pool):
          Controller(),
          pool_(pool){}
      ~PoolController() override = default;

      ObjectPool& GetPool(){
        return pool_;
      }

      bool Initialize(Router& router) override{
#define REGISTER_ENDPOINT(Method, Path, Name) \
      HTTP_CONTROLLER_##Method(Path, Name);

        FOR_EACH_POOL_CONTROLLER_ENDPOINT(REGISTER_ENDPOINT)
#undef REGISTER_ENDPOINT
        return true;
      }

      static inline PoolControllerPtr
      NewInstance(ObjectPool& pool){
        return std::make_shared<PoolController>(pool);
      }
    };
  }
}

#endif//TOKEN_HTTP_CONTROLLER_POOL_H