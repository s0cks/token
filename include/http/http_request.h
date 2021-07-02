#ifndef TOKEN_REQUEST_H
#define TOKEN_REQUEST_H

#include <uv.h>
#include <string>
#include <sstream>
#include <utility>
#include <http_parser.h>

#include "buffer.h"
#include "http/http_message.h"

namespace token{
  namespace http{
    class Request : public Message{
     private:
      Method method_;
      std::string path_;
      std::string body_;
      ParameterMap path_params_;
      ParameterMap query_params_;
     protected:
      bool Write(const BufferPtr& buffer) const override{
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
      GetHttpStatusLine(const Method& method, const std::string& path){
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
      explicit  Request():
        Message(),
        method_(),
        path_(),
        body_(),
        path_params_(),
        query_params_(){}
      Request(const HttpHeadersMap& headers,
              const Method& method,
              std::string path,
              ParameterMap  path_params,
              ParameterMap  query_params,
              std::string  body):
        Message(headers),
        method_(method),
        path_(std::move(path)),
        body_(std::move(body)),
        path_params_(std::move(path_params)),
        query_params_(std::move(query_params)){}
      ~Request() override = default;

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

      Method GetMethod() const{
        return method_;
      }

      bool IsGet() const{
        return method_ == Method::kGet;
      }

      bool IsPost() const{
        return method_ == Method::kPost;
      }

      bool IsPut() const{
        return method_ == Method::kPut;
      }

      bool IsDelete() const{
        return method_ == Method::kDelete;
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

      std::string ToString() const override{
        std::stringstream ss;
        ss << "HttpRequest(";
        ss << "path=" << path_ << ", ";
        ss << "body=" << body_;
        ss << ")";
        return ss.str();
      }

      int64_t GetBufferSize() const override{
        int64_t size = 0;
        size += GetHttpStatusLine(method_, path_).length();
        for(auto& it : headers_)
          size += GetHttpHeaderLine(it.first, it.second).length();
        size += sizeof('\r');
        size += sizeof('\n');
        size += body_.length();
        return size;
      }
    };

    class RequestBuilder : public MessageBuilderBase{
     private:
      Method method_;
      std::string path_;
      std::string body_;
      ParameterMap path_params_;
      ParameterMap query_params_;
     public:
      RequestBuilder(const Method& method, const std::string& path, const ParameterMap& path_params, const ParameterMap& query_params):
        MessageBuilderBase(),
        method_(method),
        path_(path),
        body_(),
        path_params_(path_params),
        query_params_(query_params){}
      RequestBuilder(const Method& method, const std::string& path, const ParameterMap& path_params):
        RequestBuilder(method, path, path_params, ParameterMap()){}
      RequestBuilder(const Method& method, const std::string& path):
        RequestBuilder(method, path, ParameterMap()){}
      explicit RequestBuilder():
        RequestBuilder(Method::kGet, std::string()){}
      ~RequestBuilder() override = default;

      void SetMethod(const Method& method){
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

      void SetPathParameters(const ParameterMap& params){
        path_params_.clear();
        path_params_.insert(params.begin(), params.end());
      }

      bool SetPathParameter(const std::string& name, const std::string& value){
        return path_params_.insert({ name, value }).second;
      }

      RequestPtr Build() const{
        return std::make_shared<Request>(headers_, method_, path_, path_params_, query_params_, body_);
      }
    };

    class RequestParser : public RequestBuilder{
     private:
      http_parser parser_;
      http_parser_settings settings_;

      static int
      OnParseURL(http_parser* parser, const char* data, size_t len){
        auto p = (RequestParser*)parser->data;
        p->SetPath(std::string(data, len));
        return 0;
      }

      static int
      OnParseBody(http_parser* parser, const char* data, size_t len){
        auto p = (RequestParser*)parser->data;
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
      RequestParser():
        RequestBuilder(),
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
          auto err = (http_errno)parser_.http_errno;
          LOG(WARNING) << "http parser parsed " << parsed << "/" << len << " bytes (" << http_errno_name(err) << "): " << http_errno_description(err);
          return false;
        }
        return true;
      }

      inline bool
      Parse(const BufferPtr& buffer){
        LOG_IF(WARNING, !Parse(buffer->data(), buffer->GetWrittenBytes())) << "couldn't parse request of size: " << buffer->GetWrittenBytes();
        buffer->SetReadPosition(buffer->GetReadBytes() + buffer->GetWrittenBytes());
        return true;
      }

      static inline RequestPtr
      ParseRequest(const BufferPtr& buffer){
        RequestParser parser;
        if(!parser.Parse(buffer)){
          LOG(ERROR) << "cannot parse http request.";
          return nullptr;
        }
        return parser.Build();
      }
    };

    static inline RequestPtr
    NewRequest(const Method& method, const std::string& path, const ParameterMap& path_params, const ParameterMap& query_params){
      RequestBuilder builder(method, path, path_params, query_params);
      return builder.Build();
    }

    static inline RequestPtr
    NewGetRequest(const std::string& path, const ParameterMap& path_params={}, const ParameterMap& query_params={}){
      return NewRequest(Method::kGet, path, path_params, query_params);
    }

    static inline RequestPtr
    NewPostRequest(const std::string& path, const ParameterMap& path_params={}, const ParameterMap& query_params={}){
      return NewRequest(Method::kPost, path, path_params, query_params);
    }
  }
}

#endif //TOKEN_REQUEST_H