#ifndef TOKEN_HTTP_COMMON_H
#define TOKEN_HTTP_COMMON_H

#include <memory>
#include <ostream>
#include <unordered_map>

#include "configuration.h"

namespace token{
  namespace http{
    class Controller;
    typedef std::shared_ptr<Controller> ControllerPtr;

    class Message;
    typedef std::shared_ptr<Message> MessagePtr;

    class Session;
    typedef std::shared_ptr<Session> SessionPtr;

    class Request;
    typedef std::shared_ptr<Request> RequestPtr;

    class Response;
    typedef std::shared_ptr<Response> ResponsePtr;

    class Router;
    typedef std::shared_ptr<Router> RouterPtr;

    enum class Method{
      kGet,
      kPut,
      kPost,
      kDelete,
    };

    static std::ostream& operator<<(std::ostream& stream, const Method& method){
      switch(method){
        case Method::kGet:
          return stream << "GET";
        case Method::kPut:
          return stream << "PUT";
        case Method::kPost:
          return stream << "POST";
        case Method::kDelete:
          return stream << "DELETE";
        default:
          return stream << "Unknown";
      }
    }

#define FOR_EACH_HTTP_RESPONSE(V) \
    V(Ok, 200)                      \
    V(NoContent, 204)               \
    V(NotFound, 404)                \
    V(InternalServerError, 500)     \
    V(NotImplemented, 501)

    enum StatusCode{
#define DEFINE_CODE(Name, Val) kHttp##Name = Val,
      FOR_EACH_HTTP_RESPONSE(DEFINE_CODE)
#undef DEFINE_CODE
    };

    typedef std::unordered_map<std::string, std::string> ParameterMap;

#define HTTP_HEADER_DATE "Date"
#define HTTP_HEADER_SERVER "Server"
#define HTTP_HEADER_X_NODE_ID "X-Token-NodeId"
#define HTTP_HEADER_X_NODE_VERSION "X-Token-Version"
#define HTTP_HEADER_CONNECTION "Connection"
#define HTTP_HEADER_CONTENT_TYPE "Content-Type"
#define HTTP_HEADER_CONTENT_LENGTH "Content-Length"
#define HTTP_HEADER_ACCESS_CONTROL_ALLOW_ORIGIN "Access-Control-Allow-Origin"

#define HTTP_CONTENT_TYPE_TEXT_PLAIN "text/plain; charset=utf-8"
#define HTTP_CONTENT_TYPE_APPLICATION_JSON "application/json; charset=utf-8"
#define HTTP_CONTENT_TYPE_IMAGE_PNG "image/png"

    typedef std::unordered_map<std::string, std::string> HttpHeadersMap;

    static inline bool
    SetHttpHeader(HttpHeadersMap& headers, const std::string& name, const std::string& value){
      return headers.insert({ name, value }).second;
    }

    static inline bool
    SetHttpHeader(HttpHeadersMap& headers, const std::string& name, const std::stringstream& val){
      return headers.insert({ name, val.str() }).second;
    }

    static inline bool
    SetHttpHeader(HttpHeadersMap& headers, const std::string& name, const long& val){
      std::stringstream ss;
      ss << val;
      return SetHttpHeader(headers, name, ss);
    }

    static inline bool
    SetHttpHeader(HttpHeadersMap& headers, const std::string& name, const Timestamp& time){
      return SetHttpHeader(headers, name, FormatTimestampReadable(time));
    }

    static inline std::string
    GetServerHeaderValue(){
      std::stringstream ss;
      ss << "Node/" << Version(TOKEN_MAJOR_VERSION, TOKEN_MINOR_VERSION, TOKEN_REVISION_VERSION);
      return ss.str();
    }

#define DEFAULT_NODE_ID "Node/Unknown"

    static inline std::string
    GetXNodeIDHeaderValue(){
      UUID node_id;
      if(!ConfigurationManager::GetInstance()->GetUUID(TOKEN_CONFIGURATION_NODE_ID, node_id)){
        return DEFAULT_NODE_ID;
      }

      std::stringstream ss;
      ss << "Node/" << node_id.ToStringAbbreviated();
      return ss.str();
    }

    static inline std::string
    GetXNodeVersionHeaderValue(){
      std::stringstream ss;
      ss << Version(TOKEN_MAJOR_VERSION, TOKEN_MINOR_VERSION, TOKEN_REVISION_VERSION);
      return ss.str();
    }

    static inline void
    InitHttpResponseHeaders(HttpHeadersMap& headers){
      SetHttpHeader(headers, HTTP_HEADER_CONNECTION, "close");
      SetHttpHeader(headers, HTTP_HEADER_ACCESS_CONTROL_ALLOW_ORIGIN, "*");
      SetHttpHeader(headers, HTTP_HEADER_DATE, Clock::now());
      SetHttpHeader(headers, HTTP_HEADER_SERVER, GetServerHeaderValue());
      SetHttpHeader(headers, HTTP_HEADER_X_NODE_ID, GetXNodeIDHeaderValue());
      SetHttpHeader(headers, HTTP_HEADER_X_NODE_VERSION, GetXNodeVersionHeaderValue());
    }
  }
}

#endif//TOKEN_HTTP_COMMON_H