#ifndef TOKEN_HTTP_STATUS_H
#define TOKEN_HTTP_STATUS_H

namespace token{
#define FOR_EACH_HTTP_RESPONSE(V) \
  V(Ok, 200)                      \
  V(NoContent, 204)               \
  V(NotFound, 404)                \
  V(InternalServerError, 500)     \
  V(NotImplemented, 501)

  enum HttpStatusCode{
#define DEFINE_CODE(Name, Val) kHttp##Name = Val,
    FOR_EACH_HTTP_RESPONSE(DEFINE_CODE)
#undef DEFINE_CODE
  };
}

#endif//TOKEN_HTTP_STATUS_H