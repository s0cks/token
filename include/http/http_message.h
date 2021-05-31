#ifndef TOKEN_HTTP_MESSAGE_H
#define TOKEN_HTTP_MESSAGE_H

#include <memory>
#include <utility>

#include "buffer.h"
#include "message.h"
#include "http/http_common.h"

namespace token{
  namespace http{
    class Message;
    typedef std::shared_ptr<Message> HttpMessagePtr;

    class Message : public MessageBase{
     public:
      static const int64_t kDefaultBodySize = 65536;
     protected:
      SessionPtr session_;
      HttpHeadersMap headers_;

      explicit Message(const SessionPtr& session):
        MessageBase(),
        session_(session),
        headers_(){}
      Message(const SessionPtr& session, HttpHeadersMap headers):
        MessageBase(),
        session_(session),
        headers_(std::move(headers)){}
    public:
      ~Message() override = default;

      SessionPtr GetSession() const{
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

      static MessagePtr From(const SessionPtr& session, const BufferPtr& buffer);
    };

    class MessageBuilderBase{
     protected:
      SessionPtr session_;
      HttpHeadersMap headers_;

      explicit MessageBuilderBase(const SessionPtr& session):
        session_(session),
        headers_(){}
      explicit MessageBuilderBase(const MessageBuilderBase& other) = default;
     public:
      virtual ~MessageBuilderBase() = default;

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

      MessageBuilderBase& operator=(const MessageBuilderBase& other) = default;
    };
  }

#define DEFINE_HTTP_MESSAGE(Name) \
  const char* GetName() const override{ return #Name; } \
  Type GetType() const override{ return Type::kHttp##Name; }
}

#endif//TOKEN_HTTP_MESSAGE_H