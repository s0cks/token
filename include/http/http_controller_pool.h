#ifndef TOKEN_HTTP_CONTROLLER_POOL_H
#define TOKEN_HTTP_CONTROLLER_POOL_H

#ifdef TOKEN_ENABLE_REST_SERVICE

#include "http/http_controller.h"

namespace token{
  class PoolController : HttpController{
   private:
    PoolController() = delete;

    // Blocks
    HTTP_CONTROLLER_ENDPOINT(GetBlock);
    HTTP_CONTROLLER_ENDPOINT(GetBlocks);

    // Transactions
    HTTP_CONTROLLER_ENDPOINT(GetTransaction);
    HTTP_CONTROLLER_ENDPOINT(GetTransactions);

    // Unclaimed Transactions
    HTTP_CONTROLLER_ENDPOINT(GetUnclaimedTransaction);
    HTTP_CONTROLLER_ENDPOINT(GetUnclaimedTransactions);
   public:
    ~PoolController() = delete;

    HTTP_CONTROLLER_INIT(){
      // Blocks
      HTTP_CONTROLLER_GET("/pool/blocks", GetBlocks);
      HTTP_CONTROLLER_GET("/pool/blocks/data/:hash", GetBlock);

      // Transactions
      HTTP_CONTROLLER_GET("/pool/transactions", GetTransactions);
      HTTP_CONTROLLER_GET("/pool/transactions/data/:hash", GetTransaction);

      // Unclaimed Transactions
      HTTP_CONTROLLER_GET("/pool/utxos", GetUnclaimedTransactions);
      HTTP_CONTROLLER_GET("/pool/utxos/:hash", GetUnclaimedTransaction);
    }
  };
}

#endif//TOKEN_ENABLE_REST_SERVICE
#endif//TOKEN_HTTP_CONTROLLER_POOL_H