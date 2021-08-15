#include "runtime.h"
#include "http/http_service_rest.h"

namespace token{
  namespace http{
    RestService::RestService(Runtime* runtime):
      ServiceBase(runtime->loop()),
      controller_info_(runtime),
      controller_pool_blk_(runtime->GetBlockPool()),
      controller_pool_txs_unsigned_(runtime->GetUnsignedTransactionPool()),
      controller_pool_txs_unclaimed_(runtime->GetUnclaimedTransactionPool()){
      controller_info_.Initialize(router_);
      controller_pool_blk_.Initialize(router_);
      controller_pool_txs_unsigned_.Initialize(router_);
      controller_pool_txs_unclaimed_.Initialize(router_);
    }
  }
}