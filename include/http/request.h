#ifndef TOKEN_REQUEST_H
#define TOKEN_REQUEST_H

#include <uv.h>
#include <string>
#include <sstream>
#include <json/value.h>
#include <json/writer.h>
#include <unordered_map>
#include <http_parser.h>
#include "utils/json_conversion.h"

namespace Token{
  enum HttpMethod{
    kGet,
    kPut,
    kPost,
    kDelete,
  };

  typedef std::unordered_map<std::string, std::string> ParameterMap;
  typedef std::unordered_map<std::string, std::string> HttpHeadersMap;

  class HttpSession;
  class HttpRequest{
    friend class HttpSession;
   private:
    HttpSession* session_;
    http_parser parser_;
    http_parser_settings settings_;
    std::string path_;
    ParameterMap parameters_;

    static int
    OnParseURL(http_parser* parser, const char* data, size_t len){
      HttpRequest* request = (HttpRequest*) parser->data;
      request->path_ = std::string(data, len);
      return 0;
    }
   public:
    HttpRequest(HttpSession* session, const char* data, size_t len):
      session_(session),
      parser_(),
      settings_(),
      path_(),
      parameters_(){
      parser_.data = this;
      settings_.on_url = &OnParseURL;
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

    std::string GetPath() const{
      return path_;
    }

    HttpMethod GetMethod() const{
      switch(parser_.method){
        case HTTP_GET:return HttpMethod::kGet;
        case HTTP_PUT:return HttpMethod::kPut;
        case HTTP_POST:return HttpMethod::kPost;
        case HTTP_DELETE:return HttpMethod::kDelete;
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
  };

#define STATUS_CODE_OK 200
#define STATUS_CODE_CLIENT_ERROR 400
#define STATUS_CODE_NOTFOUND 404
#define STATUS_CODE_NOTSUPPORTED 405
#define STATUS_CODE_INTERNAL_SERVER_ERROR 500

#define CONTENT_TYPE_TEXT_PLAIN "text/plain"
#define CONTENT_TYPE_APPLICATION_JSON "application/json"

  typedef int HttpStatusCode;

  class HttpResponse{
    friend class HttpSession;
   private:
    HttpSession* session_;
    HttpStatusCode status_code_;
    HttpHeadersMap headers_;

    inline std::string
    GetHttpMessageHeader() const{
      std::stringstream ss;
      ss << "HTTP/1.1 " << GetStatusCode() << " OK\r\n";
      return ss.str();
    }

    inline std::string
    GetHttpHeader(const std::string& name, const std::string& value) const{
      std::stringstream ss;
      ss << name << ": " << value << "\r\n";
      return ss.str();
    }
   protected:
    bool WriteHttp(const BufferPtr& buff) const{
      buff->PutString(GetHttpMessageHeader());
      return true;
    }

    bool WriteHeaders(const BufferPtr& buff) const{
      for(auto& it : headers_)
        buff->PutString(GetHttpHeader(it.first, it.second));
      buff->PutString("\r\n");
      return true;
    }

    virtual bool Write(const BufferPtr& buff) const = 0;
   public:
    HttpResponse(HttpSession* session, const HttpStatusCode& code):
      session_(session),
      status_code_(code){}
    virtual ~HttpResponse() = default;

    HttpSession* GetSession() const{
      return session_;
    }

    int GetStatusCode() const{
      return status_code_;
    }

    void SetHeader(const std::string& key, const std::string& value){
      auto pos = headers_.insert({key, value});
      if(!pos.second)
        LOG(WARNING) << "cannot add header " << key << ":" << value;
    }

    void SetHeader(const std::string& key, const long& value){
      std::stringstream val;
      val << value;
      auto pos = headers_.insert({key, val.str()});
      if(!pos.second)
        LOG(WARNING) << "cannot add header " << key << ":" << value;
    }

    bool HasHeaderValue(const std::string& key){
      auto pos = headers_.find(key);
      return pos != headers_.end();
    }

    std::string GetHeaderValue(const std::string& key){
      auto pos = headers_.find(key);
      return pos->second;
    }

    virtual int64_t GetContentLength() const = 0;

    int64_t GetBufferSize() const{
      int64_t size = 0;
      size += GetHttpMessageHeader().size();
      for(auto& it : headers_)
        size += GetHttpHeader(it.first, it.second).size();
      size += GetContentLength();
      return size;
    }
  };

  class HttpTextResponse : public HttpResponse{
   private:
    std::string body_;
   protected:
    bool Write(const BufferPtr& buff) const{
      WriteHttp(buff);
      WriteHeaders(buff);
      buff->PutString(body_);
      return true;
    }
   public:
    HttpTextResponse(HttpSession* session, const HttpStatusCode& status_code, const std::string& body):
      HttpResponse(session, status_code),
      body_(body){}
    ~HttpTextResponse() = default;

    int64_t GetContentLength() const{
      return body_.size();
    }
  };

  class HttpJsonResponse : public HttpResponse{
   private:
    const JsonString& body_;
   protected:
    bool Write(const BufferPtr& buffer) const{
      WriteHttp(buffer);
      WriteHeaders(buffer);
      buffer->PutBytes((uint8_t*) body_.GetString(), body_.GetSize());
      return true;
    }
   public:
    HttpJsonResponse(HttpSession* session, const HttpStatusCode& status_code, const JsonString& body):
      HttpResponse(session, status_code),
      body_(body){
      SetHeader("Content-Type", CONTENT_TYPE_APPLICATION_JSON);
      SetHeader("Content-Length", body.GetSize());
    }
    ~HttpJsonResponse() = default;

    int64_t GetContentLength() const{
      return body_.GetSize();
    }
  };
}

#endif //TOKEN_REQUEST_H