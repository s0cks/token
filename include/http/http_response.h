#ifndef TOKEN_INCLUDE_HTTP_RESPONSE_H
#define TOKEN_INCLUDE_HTTP_RESPONSE_H

#include <memory>
#include <utility>
#include <http_parser.h>

#include "block.h"
#include "http/http_common.h"
#include "http/http_message.h"

namespace token{
  namespace http{
    class Response : public Message{
      friend class HttpSession;
     protected:
      StatusCode status_;
      BufferPtr body_;

      static inline std::string
      GetHttpStatusLine(const StatusCode& status_code){
        std::stringstream ss;
        ss << "HTTP/1.1 " << status_code << " OK\r\n";
        return ss.str();
      }

      static inline std::string
      GetHttpHeaderLine(const std::string& name, const std::string& value){
        std::stringstream ss;
        ss << name << ": " << value << "\r\n";
        return ss.str();
      }
     protected:
      bool Write(const BufferPtr& buff) const override{
        if(!buff->PutString(GetHttpStatusLine(GetStatusCode())))
          return false;
        for(auto& hdr : headers_)
          if(!buff->PutString(GetHttpHeaderLine(hdr.first, hdr.second)))
            return false;
        return buff->PutString("\r\n")
            && buff->PutBytes(body_)
            && buff->PutString("\r\n");
      }

      static int
      OnParseBody(http_parser* parser, const char* data, size_t len){
        ResponsePtr response = Response::CastFrom(parser->data);
        if(!response->body_->PutBytes((uint8_t*)data, len)){
          LOG(WARNING) << "cannot put body bytes.";
          return -1;
        }
        return 0;
      }
     public:
      Response(const SessionPtr& session,
               const HttpHeadersMap& headers,
               const StatusCode& status,
               const BufferPtr& body):
         Message(session, headers),
         status_(status),
         body_(std::move(body)){}
      ~Response() override = default;

      DEFINE_HTTP_MESSAGE(Response);

      StatusCode GetStatusCode() const{
        return status_;
      }

      int64_t GetContentLength() const{
        return body_->GetWrittenBytes();
      }

      int64_t GetBufferSize() const override{
        int64_t size = 0;
        size += GetHttpStatusLine(GetStatusCode()).length();
        for(auto& it : headers_)
          size += GetHttpHeaderLine(it.first, it.second).length();
        size += sizeof('\r');
        size += sizeof('\n');
        size += GetContentLength();
        size += sizeof('\r');
        size += sizeof('\n');
        return size;
      }

      BufferPtr GetBody() const{
        return body_;
      }

      std::string GetBodyAsString() const{
        return std::string(body_->data(), body_->GetWrittenBytes());
      }

      bool GetBodyAsDocument(Json::Document& document) const{
        document.Parse(body_->data(), body_->GetWrittenBytes()).Empty();
        return true;//TODO: fix
      }

      std::string ToString() const override{
        std::stringstream ss;
        ss << "HttpResponse(";
        ss << "status_code=" << GetStatusCode() << ", ";
        ss << "headers=[], ";
        ss << "body=" << GetBodyAsString();
        ss << ")";
        return ss.str();
      }

      static inline ResponsePtr
      CastFrom(void* data){
        return std::shared_ptr<Response>((Response*)data);
      }
    };

    class ResponseBuilder : public MessageBuilderBase{
     private:
      StatusCode status_;
      BufferPtr body_;
     public:
      explicit ResponseBuilder(const SessionPtr& session):
        MessageBuilderBase(session),
        status_(StatusCode::kHttpOk),
        body_(Buffer::NewInstance(Message::kDefaultBodySize)){
        InitHttpResponseHeaders(headers_);
      }
      ResponseBuilder(const SessionPtr& session, const BufferPtr& body):
        MessageBuilderBase(session),
        status_(StatusCode::kHttpOk),
        body_(std::move(body)){
        InitHttpResponseHeaders(headers_);
      }
      ResponseBuilder(const ResponseBuilder& other):
        MessageBuilderBase(other),
        status_(other.status_),
        body_(other.body_){
        InitHttpResponseHeaders(headers_);
      }
      ~ResponseBuilder() override = default;

      void SetStatusCode(const StatusCode& status_code){
        status_ = status_code;
      }

      void SetBody(const BufferPtr& body){
        body_ = body;
      }

      void SetBody(const std::string& body){
        body_ = Buffer::CopyFrom(body);
      }

      void SetBody(const Json::String& body){
        body_ = Buffer::CopyFrom(body);
      }

      ResponsePtr Build() const{
        return std::make_shared<Response>(session_, headers_, status_, body_);
      }

      ResponseBuilder& operator=(ResponseBuilder& other) = default;
    };

    class ResponseParser : public ResponseBuilder{
     private:
      http_parser parser_;
      http_parser_settings settings_;
      std::string next_header_;

      void SetNextHeader(const std::string& next){
        next_header_ = next;
      }

      bool CreateNextHeader(const std::string& value){
        auto it = headers_.insert({ next_header_, value });
        return it.second;
      }

      static int
      OnParseBody(http_parser* p, const char* data, size_t len){
        auto parser = (ResponseParser*)p->data;
        parser->SetBody(Buffer::CopyFrom(data, len));
        return 0;
      }

      static int
      OnHeaderField(http_parser* p, const char* data, size_t len){
        auto parser = (ResponseParser*)p->data;
        parser->SetNextHeader(std::string(data, len));
        return 0;
      }

      static int
      OnHeaderValue(http_parser* p, const char* data, size_t len){
        auto parser = (ResponseParser*)p->data;
        parser->CreateNextHeader(std::string(data, len));
        return 0;
      }

      static int
      OnStatus(http_parser* parser, const char* data, size_t len){
        DLOG(INFO) << "status code: " << std::string(data, len);
        return 0;
      }
     public:
      explicit ResponseParser(const SessionPtr& session):
        ResponseBuilder(session),
        parser_(),
        settings_(){
        parser_.data = this;
        settings_.on_status = &OnStatus;
        settings_.on_body = &OnParseBody;
        settings_.on_header_field = &OnHeaderField;
        settings_.on_header_value = &OnHeaderValue;
        http_parser_init(&parser_, HTTP_RESPONSE);
      }
      ~ResponseParser() override = default;

      bool Parse(const char* data, size_t len){
        size_t parsed;
        if((parsed = http_parser_execute(&parser_, &settings_, data, len)) != len){
          LOG(WARNING) << "http parser parsed " << parsed << "/" << len << "bytes: " << http_errno_name((http_errno)parser_.http_errno) << ": " << http_errno_description((http_errno)parser_.http_errno);
          return false;
        }
        return true;
      }

      inline bool
      Parse(const BufferPtr& buffer){
        return Parse(buffer->data(), buffer->GetWrittenBytes());
      }

      static inline ResponsePtr
      ParseResponse(const SessionPtr& session, const BufferPtr& body){
        ResponseParser parser(session);
        if(!parser.Parse(body)){
          LOG(WARNING) << "cannot parse http response.";
          return nullptr;
        }
        return parser.Build();
      }
    };

    static inline ResponsePtr
    NewOkResponse(const SessionPtr& session, const Json::String& body){
      ResponseBuilder builder(session);
      builder.SetStatusCode(StatusCode::kHttpOk);
      builder.SetHeader(HTTP_HEADER_CONTENT_TYPE, HTTP_CONTENT_TYPE_APPLICATION_JSON);
      builder.SetHeader(HTTP_HEADER_CONTENT_LENGTH, body.GetSize());
      builder.SetBody(body);
      return builder.Build();
    }

    static inline ResponsePtr
    NewOkResponse(const SessionPtr& session, const std::string& msg="Ok"){
      Json::String body;
      Json::Writer writer(body);
      writer.StartObject();
      Json::SetField(writer, "data", msg);
      writer.EndObject();
      return NewOkResponse(session, body);
    }

    template<class T>
      static inline ResponsePtr
      NewOkResponse(const SessionPtr& session, const std::shared_ptr<T>& val){
        Json::String body;
        Json::Writer writer(body);
        writer.StartObject();
        writer.Key("data");
        val->Write(writer);
        writer.EndObject();
        return NewOkResponse(session, body);
      }

    static inline ResponsePtr
    NewErrorResponse(const SessionPtr& session, const StatusCode& status_code, const std::string& msg){
      ResponseBuilder builder(session);

      Json::String body;
      Json::Writer writer(body);
      writer.StartObject();
      Json::SetField(writer, "code", status_code);
      Json::SetField(writer, "message", msg);
      writer.EndObject();

      builder.SetStatusCode(status_code);
      builder.SetHeader(HTTP_HEADER_CONTENT_TYPE, HTTP_CONTENT_TYPE_APPLICATION_JSON);
      builder.SetHeader(HTTP_HEADER_CONTENT_LENGTH, body.GetSize());
      builder.SetBody(body);
      return builder.Build();
    }

    static inline ResponsePtr
    NewInternalServerErrorResponse(const SessionPtr& session, const std::string& message){
      return NewErrorResponse(session, StatusCode::kHttpInternalServerError, message);
    }

    static inline ResponsePtr
    NewInternalServerErrorResponse(const SessionPtr& session, const std::stringstream& message){
      return NewErrorResponse(session, StatusCode::kHttpInternalServerError, message.str());
    }

    static inline ResponsePtr
    NewNotImplementedResponse(const SessionPtr& session, const std::string& msg){
      return NewErrorResponse(session, StatusCode::kHttpNotImplemented, msg);
    }

    static inline ResponsePtr
    NewNotImplementedResponse(const SessionPtr&session, const std::stringstream& msg){
      return NewErrorResponse(session, StatusCode::kHttpNotImplemented, msg.str());
    }

    static inline ResponsePtr
    NewNoContentResponse(const SessionPtr& session, const std::string& msg){
      return NewErrorResponse(session, StatusCode::kHttpNoContent, msg);
    }

    static inline ResponsePtr
    NewNoContentResponse(const SessionPtr& session, const std::stringstream& msg){
      return NewErrorResponse(session, StatusCode::kHttpNoContent, msg.str());
    }

    static inline ResponsePtr
    NewNoContentResponse(const SessionPtr& session, const Hash& hash){
      std::stringstream ss;
      ss << "Cannot find: " << hash;
      return NewNoContentResponse(session, ss);
    }

    static inline ResponsePtr
    NewNotFoundResponse(const SessionPtr& session, const std::string& msg){
      return NewErrorResponse(session, StatusCode::kHttpNotFound, msg);
    }

    static inline ResponsePtr
    NewNotFoundResponse(const SessionPtr& session, const std::stringstream& msg){
      return NewErrorResponse(session, StatusCode::kHttpNotFound, msg.str());
    }

    static inline ResponsePtr
    NewNotFoundResponse(const SessionPtr& session, const Hash& hash){
      std::stringstream ss;
      ss << "Not Found: " << hash;
      return NewNotFoundResponse(session, ss);
    }

    static inline ResponsePtr
    NewNotSupportedResponse(const SessionPtr& session, const std::string& msg){
      return NewErrorResponse(session, StatusCode::kHttpNotImplemented, msg);
    }

    static inline ResponsePtr
    NewNotSupportedResponse(const SessionPtr& session, const std::stringstream& msg){
      return NewErrorResponse(session, StatusCode::kHttpNotImplemented, msg.str());
    }
  }
}

#endif //TOKEN_INCLUDE_HTTP_RESPONSE_H