#ifndef TOKEN_INCLUDE_HTTP_RESPONSE_H
#define TOKEN_INCLUDE_HTTP_RESPONSE_H

#include <memory>
#include "http/header.h"
#include "utils/json_conversion.h"

namespace Token{
  class HttpResponse;
  typedef std::shared_ptr<HttpResponse> HttpResponsePtr;

  typedef int16_t HttpStatusCode;

#define CONTENT_TYPE_TEXT_PLAIN "text/plain"
#define CONTENT_TYPE_APPLICATION_JSON "application/json"

#define STATUS_CODE_OK 200
#define STATUS_CODE_CLIENT_ERROR 400
#define STATUS_CODE_NOTFOUND 404
#define STATUS_CODE_NOTSUPPORTED 405
#define STATUS_CODE_INTERNAL_SERVER_ERROR 500

  class HttpSession;
  class HttpResponse{
    friend class HttpSession;
   private:
    HttpSession* session_;
    HttpStatusCode status_code_;
    HttpHeadersMap headers_;

    inline std::string
    GetHttpMessageHeader() const{
      std::stringstream ss;
      ss << "HTTP/1.1 " << GetStatusCode() << " OK\r\n"; //TODO: fix response code
      return ss.str();
    }

    inline std::string
    GetHttpHeader(const std::string& name, const std::string& value) const{
      std::stringstream ss;
      ss << name << ": " << value << "\r\n";
      return ss.str();
    }
   protected:
    bool WriteHttpResponseHeader(const BufferPtr& buff) const{
      buff->PutString(GetHttpMessageHeader());
      return true;
    }

    bool WriteHeaders(const BufferPtr& buff) const{
      for(auto& it : headers_)
        buff->PutString(GetHttpHeader(it.first, it.second));
      buff->PutString("\r\n");
      return true;
    }

    virtual bool Write(const BufferPtr& buff) const{
      WriteHttpResponseHeader(buff);
      WriteHeaders(buff);
      return true;
    }
   public:
    HttpResponse(HttpSession* session, const HttpStatusCode& code):
      session_(session),
      status_code_(code),
      headers_(){}
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

    std::string GetHeaderValue(const std::string& key) const{
      auto pos = headers_.find(key);
      return pos->second;
    }

    int64_t GetContentLength() const{
      std::string length = GetHeaderValue("Content-Length");
      return atoll(length.data());
    }

    int64_t GetBufferSize() const{
      int64_t size = 0;
      size += GetHttpMessageHeader().size();
      LOG(INFO) << "writing " << headers_.size() << " headers...";
      for(auto& it : headers_)
        size += GetHttpHeader(it.first, it.second).length();
      size += GetContentLength();
      return size;
    }
  };

  class HttpBinaryResponse : public HttpResponse{
   private:
    BufferPtr body_;
   protected:
    bool Write(const BufferPtr& buff) const{
      HttpResponse::Write(buff);
      buff->PutBytes(body_);
      return true;
    }
   public:
    HttpBinaryResponse(HttpSession* session,
                       const HttpStatusCode& status_code,
                       const std::string& filename,
                       const std::string& content_type = CONTENT_TYPE_TEXT_PLAIN):
      HttpResponse(session, status_code),
      body_(){
      std::fstream fd(filename, std::ios::in | std::ios::binary);
      int64_t fsize = GetFilesize(fd);
      body_ = Buffer::NewInstance(fsize);
      if(!body_->ReadBytesFrom(fd, fsize)){
        LOG(WARNING) << "couldn't read " << fsize << " bytes from: " << filename;
        SetHeader("Content-Length", 0);
      } else{
        SetHeader("Content-Length", fsize);
      }
      SetHeader("Content-Type", content_type);
    }
    ~HttpBinaryResponse() = default;

    static inline HttpResponsePtr
    NewInstance(HttpSession* session,
                const HttpStatusCode& status_code,
                const std::string& filename,
                const std::string& content_type = CONTENT_TYPE_TEXT_PLAIN){
      return std::make_shared<HttpBinaryResponse>(session, status_code, filename, content_type);
    }
  };

  class HttpTextResponse : public HttpResponse{
   private:
    const char* body_;
    int64_t length_;
   protected:
    bool Write(const BufferPtr& buff) const{
      HttpResponse::Write(buff);
      buff->PutBytes((uint8_t*)body_, length_);
      return true;
    }
   public:
    HttpTextResponse(HttpSession* session, const HttpStatusCode& status_code, const std::string& body):
      HttpResponse(session, status_code),
      body_(strdup(body.data())),
      length_(strlen(body.data())){
      SetHeader("Content-Type", CONTENT_TYPE_TEXT_PLAIN);
      SetHeader("Content-Length", body.length());
    }
    ~HttpTextResponse(){
      if(body_)
        free((void*)body_);
    }

    static inline HttpResponsePtr
    NewInstance(HttpSession* session, const HttpStatusCode& status_code, const std::string& body){
      return std::make_shared<HttpTextResponse>(session, status_code, body);
    }
  };

  class HttpJsonResponse : public HttpResponse{
   private:
    JsonString* body_;
   protected:
    bool Write(const BufferPtr& buffer) const{
      HttpResponse::Write(buffer);
      LOG(INFO) << "writing " << body_->GetLength();
      LOG(INFO) << "remaining bytes: " << buffer->GetWrittenBytes();
      buffer->PutBytes((uint8_t*)body_->GetString(), body_->GetLength());
      return true;
    }
   public:
    HttpJsonResponse(HttpSession* session, const HttpStatusCode& status_code, JsonString* body):
      HttpResponse(session, status_code),
      body_(body){
      SetHeader("Content-Type", CONTENT_TYPE_APPLICATION_JSON);
      SetHeader("Content-Length", body->GetLength());
    }
    ~HttpJsonResponse() = default;

    static inline HttpResponsePtr
    NewInstance(HttpSession* session, const HttpStatusCode& status_code, JsonString* body){
      return std::make_shared<HttpJsonResponse>(session, status_code, body);
    }
  };
}

#endif //TOKEN_INCLUDE_HTTP_RESPONSE_H