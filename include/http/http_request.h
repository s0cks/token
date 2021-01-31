#ifndef TOKEN_REQUEST_H
#define TOKEN_REQUEST_H

#include <uv.h>
#include <string>
#include <sstream>
#include <unordered_map>
#include <http_parser.h>

#include "http/http_method.h"
#include "http/http_header.h"
#include "http/http_message.h"
#include "http/http_service.h"
#include "utils/buffer.h"

namespace Token{
  class HttpRequest;
  typedef std::shared_ptr<HttpRequest> HttpRequestPtr;

  typedef std::unordered_map<std::string, std::string> ParameterMap;

  class HttpSession;
  class HttpRequest : public HttpMessage{
    friend class HttpSession;
   private:
    HttpSession* session_;
    http_parser parser_;
    http_parser_settings settings_;
    std::string path_;
    std::string body_;
    ParameterMap path_params_;
    ParameterMap query_params_;

    static int
    OnParseURL(http_parser* parser, const char* data, size_t len){
      HttpRequest* request = (HttpRequest*) parser->data;
      request->path_ = std::string(data, len);
      return 0;
    }

    static int
    OnParseBody(http_parser* parser, const char* data, size_t len){
      HttpRequest* request = (HttpRequest*)parser->data;
      request->body_ = std::string(data, len);
      return 0;
    }
   protected:
    bool Write(const BufferPtr& buffer) const{
      return false;
    }
   public:
    HttpRequest(HttpSession* session, const char* data, size_t len):
      HttpMessage(),
      session_(session),
      parser_(),
      settings_(),
      path_(),
      body_(),
      path_params_(),
      query_params_(){
      parser_.data = this;
      settings_.on_url = &OnParseURL;
      settings_.on_body = &OnParseBody;
      http_parser_init(&parser_, HTTP_REQUEST);
      size_t parsed;
      if((parsed = http_parser_execute(&parser_, &settings_, data, len)) == 0){
        LOG(WARNING) << "http parser parsed nothing";
        return;
      }
    }
    ~HttpRequest(){}

    const char* GetName() const{
      return "HttpRequest";
    }

    void SetPathParameters(const ParameterMap& params){
      path_params_.clear();
      path_params_.insert(params.begin(), params.end());
    }

    void SetQueryParameters(const ParameterMap& params){
      query_params_.clear();
      query_params_.insert(params.begin(), params.end());
    }

    std::string GetBody() const{
      return body_;
    }

    void GetBody(Json::Document& doc) const{
      doc.Parse(body_.data(), body_.length());
    }

    std::string GetPath() const{
      return path_;
    }

    HttpMethod GetMethod() const{
      switch(parser_.method){
        case HTTP_GET: return HttpMethod::kGet;
        case HTTP_PUT: return HttpMethod::kPut;
        case HTTP_POST: return HttpMethod::kPost;
        case HTTP_DELETE: return HttpMethod::kDelete;
        default:
          // should we do this?
          return HttpMethod::kGet;
      }
    }

    bool IsGet() const{
      return parser_.method == HTTP_GET;
    }

    bool IsPost() const{
      return parser_.method == HTTP_POST;
    }

    bool IsPut() const{
      return parser_.method == HTTP_PUT;
    }

    bool IsDelete() const{
      return parser_.method == HTTP_DELETE;
    }

    bool HasPathParameter(const std::string& name) const{
      auto pos = path_params_.find(name);
      return pos != path_params_.end();
    }

    bool HasQueryParameter(const std::string& name) const{
      auto pos = query_params_.find(name);
      return pos != query_params_.end();
    }

    HttpSession* GetSession() const{
      return session_;
    }

    std::string GetPathParameterValue(const std::string& name) const{
      auto pos = path_params_.find(name);
      return pos != path_params_.end() ? pos->second : "";
    }

    std::string GetQueryParameterValue(const std::string& name) const{
      auto pos = query_params_.find(name);
      return pos != query_params_.end() ? pos->second : "";
    }

    User GetUserParameterValue(const std::string& name="user") const{
      return User(GetPathParameterValue(name));
    }

    Product GetProductParameterValue(const std::string& name="product") const{
      return Product(GetPathParameterValue(name));
    }

    Hash GetHashParameterValue(const std::string& name="hash") const{
      return Hash::FromHexString(GetPathParameterValue(name));
    }

    std::string ToString() const{
      std::stringstream ss;
      ss << "HttpRequest()";
      return ss.str();
    }

    int64_t GetBufferSize() const{
      return 0;
    }

    static HttpRequestPtr
    NewInstance(HttpSession* session, const BufferPtr& buffer){
      return std::make_shared<HttpRequest>(session, buffer->data(), buffer->GetBufferSize());
    }
  };
}

#endif //TOKEN_REQUEST_H