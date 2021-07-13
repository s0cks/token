#ifndef TOKEN_HTTP_COMMON_H
#define TOKEN_HTTP_COMMON_H

#include <memory>
#include <ostream>
#include <unordered_map>

#include "json.h"
#include "version.h"
#include "timestamp.h"
#include "configuration.h"

namespace token{
  namespace http{
#define TOKEN_HTTP_VERSION "1.1"

#define FOR_EACH_HTTP_TYPE(V) \
    V(Session)                \
    V(Message)                \
    V(Request)                \
    V(Response)               \
    V(Controller)             \
    V(Router)

#define FORWARD_DECLARE_HTTP_TYPE(Name) \
    class Name;                         \
    typedef std::shared_ptr<Name> Name##Ptr;
    FOR_EACH_HTTP_TYPE(FORWARD_DECLARE_HTTP_TYPE)
#undef FORWARD_DECLARE_HTTP_TYPE

#define FOR_EACH_HTTP_METHOD(V) \
  V(Get, "GET")                 \
  V(Put, "PUT")                 \
  V(Post, "POST")               \
  V(Delete, "DELETE")

    enum class Method{
#define DEFINE_HTTP_METHOD(Name, ToString) k##Name,
      FOR_EACH_HTTP_METHOD(DEFINE_HTTP_METHOD)
#undef DEFINE_HTTP_METHOD
    };

    static inline std::ostream&
    operator<<(std::ostream& stream, const Method& method){
      switch(method){
#define DEFINE_TOSTRING(Name, ToString) \
        case Method::k##Name: return stream << ToString;
        FOR_EACH_HTTP_METHOD(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
        default:
          return stream << "UNKNOWN";
      }
    }

#define FOR_EACH_HTTP_STATUS_CODE(V) \
    V(Ok, 200)                      \
    V(NoContent, 204)               \
    V(NotFound, 404)                \
    V(InternalServerError, 500)     \
    V(NotImplemented, 501)

    enum class StatusCode{
#define DEFINE_CODE(Name, Val) k##Name = Val,
      FOR_EACH_HTTP_STATUS_CODE(DEFINE_CODE)
#undef DEFINE_CODE
    };

    static inline std::ostream&
    operator<<(std::ostream& stream, const StatusCode& status){
      switch(status){
#define DEFINE_TOSTRING(Name, ToInt) \
        case StatusCode::k##Name: return stream << #Name;
        FOR_EACH_HTTP_STATUS_CODE(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
        default:
          return stream << "Unknown";
      }
    }

    static inline std::string
    GetStatusCodeReason(const StatusCode& status){
      switch(status){
#define DEFINE_TOSTRING(Name, Code) \
        case StatusCode::k##Name: return #Name;

        FOR_EACH_HTTP_STATUS_CODE(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
        default:
          return "Unknown";
      }
    }

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
      UUID node_id = config::GetServerNodeID();

      std::stringstream ss;
      ss << "Node/" << node_id.ToString();//TODO: use to string abbreviated?
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

    struct Error{
      int64_t code;
      std::string message;
    };
  }

  namespace json{
    static inline bool
    SetField(Writer& writer, const char* name, const http::Error& error){
      if(!writer.Key(name)){
        DLOG(ERROR) << "cannot set key '" << name << "' for http::Error type.";
        return false;
      }

      if(!writer.StartObject()){
        DLOG(ERROR) << "cannot start json object for http::Error type.";
        return false;
      }

      if(!json::SetField(writer, "code", error.code)){
        DLOG(ERROR) << "cannot set code field for http::Error type.";
        return false;
      }

      if(!json::SetField(writer, "message", error.message)){
        DLOG(ERROR) << "cannot set message field for http::Error type.";
        return false;
      }

      if(!writer.EndObject()){
        DLOG(ERROR) << "cannot end json object for http::Error type.";
        return false;
      }
      return true;
    }
  }
}

#endif//TOKEN_HTTP_COMMON_H