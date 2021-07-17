#ifndef TOKEN_RPC_SERVER_H
#define TOKEN_RPC_SERVER_H

#include <memory>

#include "env.h"
#include "server.h"
#include "network/rpc_server_session.h"

namespace token{
  namespace env{
#define TOKEN_ENVIRONMENT_VARIABLE_SERVER_CALLBACK_ADDRESS "TOKEN_CALLBACK_ADDRESS"

    static inline NodeAddress
    GetDefaultServerCallbackAddress(){
      std::stringstream ss;
      ss << "localhost:" << FLAGS_server_port;
      return NodeAddress::ResolveAddress(ss.str());
    }

    static inline NodeAddress
    GetServerCallbackAddress(){
      if(!env::HasVariable(TOKEN_ENVIRONMENT_VARIABLE_SERVER_CALLBACK_ADDRESS))
        return GetDefaultServerCallbackAddress();
      return NodeAddress::ResolveAddress(env::GetString(TOKEN_ENVIRONMENT_VARIABLE_SERVER_CALLBACK_ADDRESS));
    }
  }
}

#endif//TOKEN_RPC_SERVER_H