#ifndef TOKEN_INCLUDE_HTTP_RESPONSE_H
#define TOKEN_INCLUDE_HTTP_RESPONSE_H

#include <memory>
#include <http_parser.h>

#include "block.h"
#include "wallet.h"
#include "http_common.h"
#include "http_message.h"

namespace token{
  namespace http{
    class Response : public Message{
     protected:
      StatusCode status_;
      BufferPtr body_;

      static inline std::string
      GetHttpStatusLine(const StatusCode& status_code){
        std::stringstream ss;
        ss << "HTTP/" << TOKEN_HTTP_VERSION << " ";
        ss << static_cast<int64_t>(status_code) << " " << GetStatusCodeReason(status_code) << "\r\n";
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
      Response(const HttpHeadersMap& headers,
               const StatusCode& status,
               const BufferPtr& body):
         Message(headers),
         status_(status),
         body_(std::move(body)){}
      ~Response() override = default;

      DEFINE_HTTP_MESSAGE(Response);

      StatusCode GetStatusCode() const{
        return status_;
      }

      int64_t GetContentLength() const{
        return body_->GetWritePosition();
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

      BufferPtr ToBuffer() const{
        BufferPtr buffer = internal::NewBuffer(GetBufferSize());
        LOG_IF(ERROR, !Write(buffer)) << "couldn't write response to buffer.";
        return buffer;
      }

      BufferPtr GetBody() const{
        return body_;
      }

      std::string GetBodyAsString() const{
        return std::string((char*)body_->data(), body_->GetWritePosition());
      }

      bool GetBodyAsDocument(json::Document& document) const{
        document.Parse((char*)body_->data(), body_->GetWritePosition()).Empty();
        return true;//TODO: fix
      }

      std::string ToString() const override{
        if(VLOG_IS_ON(1)){
          BufferPtr body = internal::NewBuffer(GetBufferSize());
          if(!Write(body))
            VLOG(1) << "cannot write body to buffer.";
          return std::string((char*)body->data(), body->GetWritePosition());
        }

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
      ResponseBuilder():
        MessageBuilderBase(),
        status_(StatusCode::kOk),
        body_(internal::NewBuffer(kMessageDefaultBodySize)){
        InitHttpResponseHeaders(headers_);
      }
      explicit ResponseBuilder(const BufferPtr& body):
        MessageBuilderBase(),
        status_(StatusCode::kOk),
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
        body_ = internal::CopyBufferFrom(body);
      }

      void SetBody(const json::String& body){
        body_ = internal::CopyBufferFrom(body);
      }

      ResponsePtr Build() const{
        return std::make_shared<Response>(headers_, status_, body_);
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
        parser->SetBody(internal::CopyBufferFrom((uint8_t *) data, static_cast<int64_t>(len)));
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
      ResponseParser():
        ResponseBuilder(),
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
        return Parse((char*)buffer->data(), buffer->GetWritePosition());
      }

      static inline ResponsePtr
      ParseResponse(const BufferPtr& body){
        ResponseParser parser;
        if(!parser.Parse(body)){
          LOG(WARNING) << "cannot parse http response.";
          return nullptr;
        }
        return parser.Build();
      }
    };

    static inline ResponsePtr
    NewOkResponse(const json::String& body){
      ResponseBuilder builder;
      builder.SetStatusCode(StatusCode::kOk);
      builder.SetHeader(HTTP_HEADER_CONTENT_TYPE, HTTP_CONTENT_TYPE_APPLICATION_JSON);
      builder.SetHeader(HTTP_HEADER_CONTENT_LENGTH, body.GetSize());
      builder.SetBody(body);
      return builder.Build();
    }

    static inline ResponsePtr
    NewOkResponse(const std::string& msg="Ok"){
      json::String body;
      json::Writer writer(body);

      LOG_IF(WARNING, !writer.StartObject()) << "cannot start json object.";
      LOG_IF(WARNING, !json::SetField(writer, "data", http::Error{ 200, msg }));
      LOG_IF(WARNING, !writer.EndObject()) << "cannot end json object.";

      DLOG(INFO) << "response body: " << body.GetString();
      return NewOkResponse(body);
    }

    template<class T>
    static inline ResponsePtr
    NewOkResponse(const std::shared_ptr<T>& val){
      json::String body;
      json::Writer writer(body);

      LOG_IF(WARNING, !writer.StartObject()) << "cannot start json object.";
      LOG_IF(WARNING, !json::SetField(writer, "data", val)) << "cannot set 'data' field.";
      LOG_IF(WARNING, !writer.EndObject()) << "cannot end json object.";
      return NewOkResponse(body);
    }

    static inline ResponsePtr
    NewOkResponse(const HashList& val){
      json::String body;
      json::Writer writer(body);
      LOG_IF(WARNING, !writer.StartObject()) << "cannot start json object.";
      LOG_IF(WARNING, !json::SetField(writer, "data", val)) << "cannot set 'data' field.";
      LOG_IF(WARNING, !writer.EndObject()) << "cannot end json object.";
      return NewOkResponse(body);
    }

    static inline ResponsePtr
    NewOkResponse(const User& user, const Wallet& wallet){
      json::String body;
      json::Writer writer(body);

      DLOG_IF(ERROR, !writer.StartObject()) << "cannot start json object.";
      DLOG_IF(ERROR, !json::SetField(writer, "data", wallet));
      DLOG_IF(ERROR, !writer.EndObject()) << "cannot end json object.";
      return NewOkResponse(body);
    }

    static inline ResponsePtr
    NewErrorResponse(const StatusCode& status_code, const json::String& body){
      ResponseBuilder builder;
      builder.SetStatusCode(status_code);
      builder.SetHeader(HTTP_HEADER_CONTENT_TYPE, HTTP_CONTENT_TYPE_APPLICATION_JSON);
      builder.SetHeader(HTTP_HEADER_CONTENT_LENGTH, body.GetSize());
      builder.SetBody(body);
      return builder.Build();
    }

    static inline ResponsePtr
    NewErrorResponse(const StatusCode& status_code=StatusCode::kInternalServerError, const std::string& msg="Not Ok"){
      json::String body;
      json::Writer writer(body);
      LOG_IF(ERROR, !writer.StartObject()) << "cannot start json object.";
      LOG_IF(ERROR, !json::SetField(writer, "data", Error{ static_cast<int64_t>(status_code), msg }));
      LOG_IF(ERROR, !writer.EndObject()) << "cannot end json object.";
      return NewErrorResponse(status_code, body);
    }

    static inline ResponsePtr
    NewInternalServerErrorResponse(const std::string& message){
      return NewErrorResponse(StatusCode::kInternalServerError, message);
    }

    static inline ResponsePtr
    NewInternalServerErrorResponse(const std::stringstream& message){
      return NewErrorResponse( StatusCode::kInternalServerError, message.str());
    }

    static inline ResponsePtr
    NewNotImplementedResponse(const std::string& msg){
      return NewErrorResponse(StatusCode::kNotImplemented, msg);
    }

    static inline ResponsePtr
    NewNotImplementedResponse(const std::stringstream& msg){
      return NewErrorResponse(StatusCode::kNotImplemented, msg.str());
    }

    static inline ResponsePtr
    NewNoContentResponse(const std::string& msg){
      return NewErrorResponse(StatusCode::kNoContent, msg);
    }

    static inline ResponsePtr
    NewNoContentResponse(const std::stringstream& msg){
      return NewErrorResponse(StatusCode::kNoContent, msg.str());
    }

    static inline ResponsePtr
    NewNoContentResponse(const Hash& hash){
      std::stringstream ss;
      ss << "Cannot find: " << hash;
      return NewNoContentResponse(ss);
    }

    static inline ResponsePtr
    NewNotFoundResponse(const std::string& msg){
      return NewErrorResponse(StatusCode::kNotFound, msg);
    }

    static inline ResponsePtr
    NewNotFoundResponse(const std::stringstream& msg){
      return NewErrorResponse(StatusCode::kNotFound, msg.str());
    }

    static inline ResponsePtr
    NewNotFoundResponse(const Hash& hash){
      std::stringstream ss;
      ss << "Not Found: " << hash;
      return NewNotFoundResponse(ss);
    }

    static inline ResponsePtr
    NewNotSupportedResponse(const std::string& msg){
      return NewErrorResponse(StatusCode::kNotImplemented, msg);
    }

    static inline ResponsePtr
    NewNotSupportedResponse(const std::stringstream& msg){
      return NewErrorResponse(StatusCode::kNotImplemented, msg.str());
    }
  }
}

#endif //TOKEN_INCLUDE_HTTP_RESPONSE_H