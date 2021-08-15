#ifndef TOKEN_HTTP_SERVICE_REST_H
#define TOKEN_HTTP_SERVICE_REST_H

#include "http/http_service.h"
#include "http/http_controller_info.h"
#include "http/http_controller_pool.h"

namespace token{
  class Runtime;
  namespace http{
    class RestService : public ServiceBase{
     private:
      InfoController controller_info_;
      BlockPoolController controller_pool_blk_;
      UnclaimedTransactionPoolController controller_pool_txs_unclaimed_;
      UnsignedTransactionPoolController controller_pool_txs_unsigned_;
     public:
      explicit RestService(Runtime* runtime);
      ~RestService() override = default;

      InfoController& GetInfoController(){
        return controller_info_;
      }

      BlockPoolController& GetBlockPoolController(){
        return controller_pool_blk_;
      }

      UnclaimedTransactionPoolController& GetUnclaimedTransactionPoolController(){
        return controller_pool_txs_unclaimed_;
      }

      UnsignedTransactionPoolController& GetUnsignedTransactionPoolController(){
        return controller_pool_txs_unsigned_;
      }

      static inline bool
      IsEnabled(){
        return IsValidPort(RestService::GetPort());
      }

      static inline ServerPort
      GetPort(){
        return FLAGS_service_port;
      }

      static inline const char*
      GetName(){
        return "http/rest";
      }
    };
  }
}

#endif//TOKEN_HTTP_SERVICE_REST_H