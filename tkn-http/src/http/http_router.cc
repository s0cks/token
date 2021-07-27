#include "http/http_router.h"
#include "http/http_request.h"

namespace token{
  namespace http{
    RouterMatch Router::Find(const RequestPtr& request){
      return Search((Node*)GetRoot(), request->GetMethod(), request->GetPath());
    }
  }
}