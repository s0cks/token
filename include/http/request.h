#ifndef TOKEN_REQUEST_H
#define TOKEN_REQUEST_H

#include <uv.h>
#include <string>
#include <sstream>
#include <unordered_map>
#include <http_parser.h>

#include "http/method.h"
#include "http/header.h"
#include "utils/buffer.h"
#include "utils/json_conversion.h"

namespace Token{
  class HttpRequest;
  typedef std::shared_ptr<HttpRequest> HttpRequestPtr;

  typedef std::unordered_map<std::string, std::string> ParameterMap;

  class HttpSession;
  class HttpRequest{
    friend class HttpSession;
   private:
    HttpSession* session_;
    http_parser parser_;
    http_parser_settings settings_;
    std::string path_;
    std::string body_;
    ParameterMap parameters_;

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
   public:
    HttpRequest(HttpSession* session, const char* data, size_t len):
      session_(session),
      parser_(),
      settings_(),
      path_(),
      body_(),
      parameters_(){
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

    void SetParameters(const ParameterMap& parameters){
      parameters_.clear();
      parameters_.insert(parameters.begin(), parameters.end());
    }

    std::string GetBody() const{
      return body_;
    }

    void GetBody(JsonDocument& doc) const{
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

    bool HasParameter(const std::string& name) const{
      auto pos = parameters_.find(name);
      return pos != parameters_.end();
    }

    HttpSession* GetSession() const{
      return session_;
    }

    std::string GetParameterValue(const std::string& name) const{
      auto pos = parameters_.find(name);
      return pos != parameters_.end() ? pos->second : "";
    }

    User GetUserParameterValue(const std::string& name="user") const{
      return User(GetParameterValue(name));
    }

    Product GetProductParameterValue(const std::string& name="product") const{
      return Product(GetParameterValue(name));
    }

    Hash GetHashParameterValue(const std::string& name="hash") const{
      return Hash::FromHexString(GetParameterValue(name));
    }

    static HttpRequestPtr
    NewInstance(HttpSession* session, const char* data, size_t len){
      return std::make_shared<HttpRequest>(session, data, len);
    }
  };
}

#endif //TOKEN_REQUEST_H