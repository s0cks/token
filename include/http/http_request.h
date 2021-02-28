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

  class HttpSession;
  class HttpRequest : public HttpMessage{
    friend class HttpSession;
   private:
    HttpMethod method_;
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
      HttpMessage(session),
      method_(),
      path_(),
      body_(),
      path_params_(),
      query_params_(){}
    HttpRequest(HttpSession* session, const HttpHeadersMap& headers, const HttpMethod& method, const std::string& path, const std::string& body):
      HttpMessage(session, headers),
      method_(method),
      path_(path),
      body_(body),
      path_params_(),
      query_params_(){}
    ~HttpRequest(){}

    DEFINE_HTTP_MESSAGE(Request);

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

    static inline HttpRequestPtr
    FromJson(HttpSession* session, const HttpHeadersMap& headers, const HttpMethod& method, const std::string& path, const Json::String& body){
      return std::make_shared<HttpRequest>(session, headers, method, path, std::string(body.GetString(), body.GetSize()));
    }
  };

  class HttpRequestBuilder : public HttpMessageBuilder<HttpRequest>{
   private:
    HttpMethod method_;
    std::string path_;
    std::string body_;
    ParameterMap path_params_;
    ParameterMap query_params_;
   public:
    HttpRequestBuilder(HttpSession* session):
      HttpMessageBuilder(session),
      method_(HttpMethod::kGet),
      path_(),
      body_(),
      path_params_(),
      query_params_(){}
    ~HttpRequestBuilder() = default;

    void SetMethod(const HttpMethod& method){
      method_ = method;
    }

    void SetPath(const std::string& path){
      path_ = path;
    }

    void SetBody(const std::string& val){
      body_ = val;
    }

    void SetBody(const Json::String& val){
      body_ = std::string(val.GetString(), val.GetSize());
    }

    HttpRequestPtr Build() const{
      return std::make_shared<HttpRequest>(session_, headers_, method_, path_, body_);
    }
  };

  class HttpRequestParser : public HttpRequestBuilder{
   private:
    http_parser parser_;
    http_parser_settings settings_;

    static int
    OnParseURL(http_parser* parser, const char* data, size_t len){
      HttpRequestParser* p = (HttpRequestParser*)parser->data;
      p->SetPath(std::string(data, len));
      return 0;
    }

    static int
    OnParseBody(http_parser* parser, const char* data, size_t len){
      HttpRequestParser* p = (HttpRequestParser*)parser->data;
      p->SetBody(std::string(data, len));
      return 0;
    }

    static int
    OnParseStatus(http_parser* parser, const char* data, size_t len){
      //HttpRequestParser* p = (HttpRequestParser*)parser->data;
      LOG(INFO) << "parsed status: " << std::string(data, len);
      return 0;
    }
   public:
    HttpRequestParser(HttpSession* session):
      HttpRequestBuilder(session),
      parser_(),
      settings_(){
      settings_.on_status = &OnParseStatus;
      settings_.on_body = &OnParseBody;
      settings_.on_url = &OnParseURL;

      parser_.data = this;
      http_parser_init(&parser_, HTTP_REQUEST);
    }

    bool Parse(const char* data, size_t len){
      size_t parsed;
      if((parsed = http_parser_execute(&parser_, &settings_, data, len)) != len){
        http_errno err = (http_errno)parser_.http_errno;
        LOG(WARNING) << "http parser parsed " << parsed << "/" << len << " bytes (" << http_errno_name(err) << "): " << http_errno_description(err);
        return false;
      }
      return true;
    }

    inline bool
    Parse(const BufferPtr& buffer){
      return Parse(buffer->data(), buffer->GetWrittenBytes());
    }

    static inline HttpRequestPtr
    ParseRequest(HttpSession* session, const BufferPtr& buffer){
      HttpRequestParser parser(session);
      if(!parser.Parse(buffer)){
        LOG(ERROR) << "cannot parse http request.";
        return nullptr;
      }
      return parser.Build();
    }
  };
}

#endif //TOKEN_REQUEST_H