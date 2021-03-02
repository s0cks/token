#ifndef TOKEN_HTTP_MESSAGE_H
#define TOKEN_HTTP_MESSAGE_H

#include <memory>
#include "message.h"
#include "utils/buffer.h"

#include "http/http_header.h"
#include "http/http_status.h"

namespace token{
  template<class M>
  class Session;

  class HttpSession;

  class HttpMessage;
  typedef std::shared_ptr<HttpMessage> HttpMessagePtr;

#define DEFINE_HTTP_MESSAGE(Name) \
  const char* GetName() const{ return #Name; } \
  Type GetType() const{ return Type::kHttp##Name; }

  class HttpMessage : public Message{
   public:
    static const int64_t kDefaultBodySize = 65536;
   protected:
    HttpSession* session_;
    HttpHeadersMap headers_;

    HttpMessage(HttpSession* session):
      Message(),
      session_(session),
      headers_(){}
    HttpMessage(HttpSession* session, const HttpHeadersMap& headers):
      Message(),
      session_(session),
      headers_(headers){}
   public:
    virtual ~HttpMessage() = default;

    HttpSession* GetSession() const{
      return session_;
    }

    bool SetHeader(const std::string& name, const std::string& val){
      return SetHttpHeader(headers_, name, val);
    }

    bool SetHeader(const std::string& name, const std::stringstream& val){
      return SetHttpHeader(headers_, name, val);
    }

    bool SetHeader(const std::string& name, const int64_t& val){
      return SetHttpHeader(headers_, name, val);
    }

    bool SetHeader(const std::string& name, const Timestamp& val){
      return SetHttpHeader(headers_, name, val);
    }

    bool HasHeader(const std::string& name) const{
      auto pos = headers_.find(name);
      return pos != headers_.end();
    }

    std::string GetHeader(const std::string& name) const{
      auto pos = headers_.find(name);
      return pos->second;
    }

    virtual const char* GetName() const = 0;

    static HttpMessagePtr From(Session<HttpMessage>* session, const BufferPtr& buffer);
  };

  template<class M>
  class HttpMessageBuilder{
   protected:
    HttpSession* session_;
    HttpHeadersMap headers_;

    HttpMessageBuilder(HttpSession* session):
      session_(session),
      headers_(){}
    HttpMessageBuilder(const HttpMessageBuilder<M>& other):
      session_(other.session_),
      headers_(other.headers_){}
   public:
    virtual ~HttpMessageBuilder() = default;

    void SetHeader(const std::string& name, const std::string& val){
      if(!SetHttpHeader(headers_, name, val))
        LOG(WARNING) << "cannot set header '" << name << "' to '" << val << "'";
    }

    void SetHeader(const std::string& name, const std::stringstream& val){
      if(!SetHttpHeader(headers_, name, val))
        LOG(WARNING) << "cannot set header '" << name << "' to '" << val.str() << "'";
    }

    void SetHeader(const std::string& name, const int64_t& val){
      if(!SetHttpHeader(headers_, name, val))
        LOG(WARNING) << "cannot set header '" << name << "' to '" << val << "'";
    }

    void SetHeader(const std::string& name, const Timestamp& val){
      if(!SetHttpHeader(headers_, name, val))
        LOG(WARNING) << "cannot set header '" << name << "' to '" << FormatTimestampReadable(val) << "'";
    }

    virtual std::shared_ptr<M> Build() const = 0;

    void operator=(const HttpMessageBuilder<M>& other){
      session_ = other.session_;
      headers_ = other.headers_;
    }
  };
}

#endif//TOKEN_HTTP_MESSAGE_H