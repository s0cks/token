#ifndef TOKEN_HTTP_CONTROLLER_POOL_H
#define TOKEN_HTTP_CONTROLLER_POOL_H

#include "object_pool.h"
#include "http/http_controller.h"

namespace token{
  namespace http{
#define FOR_EACH_BLOCK_POOL_CONTROLLER_ENDPOINT(V) \
  V(GET, "/pool/blocks", GetAll)                   \
  V(GET, "/pool/blocks/data/:hash", Get)           \
  V(GET, "/pool/blocks/stats", GetStats)           \

    class BlockPoolController : public Controller{
    protected:
      BlockPool& pool_;

      inline BlockPool&
      pool() const{
        return pool_;
      }
    public:
      explicit BlockPoolController(BlockPool& pool):
        Controller(),
        pool_(pool){}
      ~BlockPoolController() override = default;
      DECLARE_HTTP_CONTROLLER(BlockPoolController, FOR_EACH_BLOCK_POOL_CONTROLLER_ENDPOINT);
    };

#define FOR_EACH_UNSIGNED_TRANSACTION_POOL_CONTROLLER_ENDPOINT(V) \
    V(GET, "/pool/unsigned_transactions", GetAll)                 \
    V(GET, "/pool/unsigned_transactions/data/:hash", Get)         \
    V(GET, "/pool/unsigned_transactions/stats", GetStats)

    class UnsignedTransactionPoolController : public Controller{
    protected:
      UnsignedTransactionPool& pool_;

      inline UnsignedTransactionPool&
      pool() const{
        return pool_;
      }
    public:
      explicit UnsignedTransactionPoolController(UnsignedTransactionPool& pool):
        Controller(),
        pool_(pool){}
      ~UnsignedTransactionPoolController() override = default;
      DECLARE_HTTP_CONTROLLER(UnsignedTransactionPoolController, FOR_EACH_UNSIGNED_TRANSACTION_POOL_CONTROLLER_ENDPOINT);
    };

#define FOR_EACH_UNCLAIMED_TRANSACTION_POOL_CONTROLLER_ENDPOINT(V) \
    V(GET, "/pool/unclaimed_transactions", GetAll)                 \
    V(GET, "/pool/unclaimed_transactions/data/:hash", Get)         \
    V(GET, "/pool/unclaimed_transactions/stats", GetStats)

    class UnclaimedTransactionPoolController : public Controller{
    protected:
      UnclaimedTransactionPool& pool_;

      inline UnclaimedTransactionPool&
      pool() const{
        return pool_;
      }
    public:
      explicit UnclaimedTransactionPoolController(UnclaimedTransactionPool& pool):
        Controller(),
        pool_(pool){}
      ~UnclaimedTransactionPoolController() override = default;
      DECLARE_HTTP_CONTROLLER(UnclaimedTransactionPoolController, FOR_EACH_UNCLAIMED_TRANSACTION_POOL_CONTROLLER_ENDPOINT);
    };
  }
}

#endif//TOKEN_HTTP_CONTROLLER_POOL_H