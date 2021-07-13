#ifndef TOKEN_HTTP_MESSAGE_H
#define TOKEN_HTTP_MESSAGE_H

#include <memory>
#include <utility>

#include "buffer.h"
#include "message.h"
#include "http_common.h"

namespace token{
  namespace http{
    class Message;
    typedef std::shared_ptr<Message> HttpMessagePtr;

    static const int64_t kMessageDefaultBodySize = 65536;
    class Message : public MessageBase{
     protected:
      HttpHeadersMap headers_;

      Message():
        MessageBase(),
        headers_(){}
      explicit Message(const HttpHeadersMap& headers):
        MessageBase(),
        headers_(headers){}
    public:
      ~Message() override = default;

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

      static MessagePtr From(const BufferPtr& buffer);
    };

    class MessageBuilderBase{
     protected:
      HttpHeadersMap headers_;

      explicit MessageBuilderBase():
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
  Type type() const override{ return Type::kHttp##Name; }
}

#endif//TOKEN_HTTP_MESSAGE_H