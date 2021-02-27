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

namespace token{
  class HttpRequest;
  typedef std::shared_ptr<HttpRequest> HttpRequestPtr;

  typedef std::unordered_map<std::string, std::string> ParameterMap;

  class HttpRequestParser{
   private:
    http_parser parser_;
    http_parser_settings settings_;
    std::string path_;
    std::string body_;
    HttpHeadersMap headers_;

    static int
    OnParseURL(http_parser* parser, const char* data, size_t len){
      HttpRequestParser* p = (HttpRequestParser*)parser->data;
      p->path_ = std::string(data, len);
      return 0;
    }

    static int
    OnParseBody(http_parser* parser, const char* data, size_t len){
      HttpRequestParser* p = (HttpRequestParser*)parser->data;
      p->body_ = std::string(data, len);
      return 0;
    }
   public:
    HttpRequestParser():
      parser_(),
      settings_(),
      path_(),
      body_(),
      headers_(){
      parser_.data = this;
      http_parser_init(&parser_, HTTP_REQUEST);
    }

    HttpRequestPtr Parse(HttpSession* session, const char* data, size_t len){
      size_t parsed;
      if((parsed = http_parser_execute(&parser_, &settings_, data, len)) != len){
        LOG(WARNING) << "http parser parsed nothing: " << http_errno_name((http_errno)parser_.http_errno) << ") " << http_errno_description((http_errno)parser_.http_errno);
        return nullptr;
      }
      return std::make_shared<HttpRequest>(nullptr);
    }

    inline HttpRequestPtr
    Parse(HttpSession* session, const BufferPtr& buffer){
      return Parse(session, buffer->data(), buffer->GetWrittenBytes());
    }
  };

  class HttpSession;
  class HttpRequest : public HttpMessage{
    friend class HttpSession;
   private:
    HttpSession* session_;
    HttpMethod method_;
    HttpHeadersMap headers_;
    std::string path_;
    std::string body_;
    ParameterMap path_params_;
    ParameterMap query_params_;
   protected:
    bool Write(const BufferPtr& buffer) const{
      if(!buffer->PutString(GetHttpStatusLine(method_, path_)))
        return false;
      for(auto& hdr : headers_){
        if(!buffer->PutString(GetHttpHeaderLine(hdr.first, hdr.second)))
          return false;
      }
      return buffer->PutString("\r\n")
          && buffer->PutString(body_);
    }

    static inline std::string
    GetHttpStatusLine(const HttpMethod& method, const std::string& path){
      std::stringstream ss;
      ss << method << " " << path << " HTTP/1.1\r\n";
      return ss.str();
    }

    static inline std::string
    GetHttpHeaderLine(const std::string& name, const std::string& value){
      std::stringstream ss;
      ss << name << ": " << value << "\r\n";
      return ss.str();
    }
   public:
    HttpRequest(HttpSession* session):
      HttpMessage(),
      session_(session),
      method_(),
      headers_(),
      path_(),
      body_(),
      path_params_(),
      query_params_(){}
    HttpRequest(HttpSession* session, const HttpMethod& method, const std::string& path, const std::string& body):
      HttpMessage(),
      session_(session),
      method_(method),
      headers_(),
      path_(path),
      body_(body),
      path_params_(),
      query_params_(){}
    ~HttpRequest(){}

    HttpHeadersMap& GetHeaders(){
      return headers_;
    }

    Type GetType() const{
      return Type::kHttpRequest;
    }

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
      return method_;
    }

    bool IsGet() const{
      return method_ == HttpMethod::kGet;
    }

    bool IsPost() const{
      return method_ == HttpMethod::kPost;
    }

    bool IsPut() const{
      return method_ == HttpMethod::kPut;
    }

    bool IsDelete() const{
      return method_ == HttpMethod::kDelete;
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
      ss << "HttpRequest(";
      ss << "path=" << path_ << ", ";
      ss << "body=" << body_;
      ss << ")";
      return ss.str();
    }

    int64_t GetBufferSize() const{
      int64_t size = 0;
      size += GetHttpStatusLine(method_, path_).length();
      for(auto& it : headers_)
        size += GetHttpHeaderLine(it.first, it.second).length();
      size += sizeof('\r');
      size += sizeof('\n');
      size += body_.length();
      return size;
    }

    static HttpRequestPtr
    NewInstance(HttpSession* session, const BufferPtr& buffer){
      HttpRequestParser parser;
      return parser.Parse(session, buffer);
    }

    static inline HttpRequestPtr
    FromJson(HttpSession* session, const HttpMethod& method, const std::string& path, const Json::String& body){
      return std::make_shared<HttpRequest>(session, method, path, std::string(body.GetString(), body.GetSize()));
    }
  };
}

#endif //TOKEN_REQUEST_H