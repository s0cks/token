#ifndef TOKEN_HTTP_CONTROLLER_WALLET_H
#define TOKEN_HTTP_CONTROLLER_WALLET_H

#include "wallet_manager.h"
#include "http/http_controller.h"

namespace token{
  namespace http{
    class WalletController;
    typedef std::shared_ptr<WalletController> WalletControllerPtr;

#define FOR_EACH_WALLET_CONTROLLER_ENDPOINT(V) \
  V(GET, "/wallet/:user", GetUserWallet)       \
  V(GET, "/wallet/:user/tokens/:hash", GetUserWalletTokenCode) \
  V(POST, "/wallet/:user/spend", PostUserWalletSpend)

    class WalletController : public Controller,
                             public std::enable_shared_from_this<WalletController>{
     protected:
      WalletManagerPtr wallets_;
     public:
      WalletController(const WalletManagerPtr& wallets):
          Controller(),
          wallets_(wallets){}
      virtual ~WalletController() = default;

      WalletManagerPtr GetWalletManager() const{
        return wallets_;
      }

#define DECLARE_ENDPOINT(Method, Path, Name) \
    HTTP_CONTROLLER_ENDPOINT(Name);

      FOR_EACH_WALLET_CONTROLLER_ENDPOINT(DECLARE_ENDPOINT)
#undef DECLARE_ENDPOINT

      bool Initialize(const RouterPtr& router){
#define REGISTER_ENDPOINT(Method, Path, Name) \
      HTTP_CONTROLLER_##Method(Path, Name);

        FOR_EACH_WALLET_CONTROLLER_ENDPOINT(REGISTER_ENDPOINT)
#undef REGISTER_ENDPOINT
        return true;
      }

      static inline WalletControllerPtr
      NewInstance(const WalletManagerPtr& wallets=WalletManager::GetInstance()){
        return std::make_shared<WalletController>(wallets);
      }
    };
  }
}

#endif//TOKEN_HTTP_CONTROLLER_WALLET_H