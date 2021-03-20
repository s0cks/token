#ifndef TOKEN_INCLUDE_HTTP_RESPONSE_H
#define TOKEN_INCLUDE_HTTP_RESPONSE_H

#include <memory>
#include <http_parser.h>


#include "block.h"
#include "http_status.h"
#include "http_header.h"
#include "http_service.h"

namespace token{
  class HttpResponse;
  typedef std::shared_ptr<HttpResponse> HttpResponsePtr;

  class HttpJsonResponse;
  typedef std::shared_ptr<HttpJsonResponse> HttpJsonResponsePtr;

  class HttpSession;
  class HttpResponse : public HttpMessage{
    friend class HttpSession;
   protected:
    HttpStatusCode status_code_;
    BufferPtr body_;

    static inline std::string
    GetHttpStatusLine(const HttpStatusCode& status_code){
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
    virtual bool Write(const BufferPtr& buff) const{
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
      HttpResponse* response = (HttpResponse*)parser->data;
      if(!response->body_->PutBytes((uint8_t*)data, len)){
        LOG(WARNING) << "cannot put body bytes.";
        return -1;
      }
      return 0;
    }
   public:
    HttpResponse(HttpSession* session, const HttpHeadersMap& headers, const HttpStatusCode& status_code, const BufferPtr& body):
      HttpMessage(session, headers),
      status_code_(status_code),
      body_(body){}
    virtual ~HttpResponse() = default;

    DEFINE_HTTP_MESSAGE(Response);

    HttpStatusCode GetStatusCode() const{
      return status_code_;
    }

    int64_t GetContentLength() const{
      return body_->GetWrittenBytes();
    }

    int64_t GetBufferSize() const{
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

    std::string ToString() const{
      std::stringstream ss;
      ss << "HttpResponse(";
        ss << "status_code=" << status_code_ << ", ";
        ss << "headers=[], ";
        ss << "body=" << GetBodyAsString();
      ss << ")";
      return ss.str();
    }
  };

  class HttpResponseBuilder : public HttpMessageBuilder<HttpResponse>{
   private:
    HttpStatusCode status_code_;
    BufferPtr body_;
   public:
    HttpResponseBuilder(HttpSession* session):
      HttpMessageBuilder(session),
      status_code_(HttpStatusCode::kHttpOk),
      body_(Buffer::NewInstance(HttpMessage::kDefaultBodySize)){
      InitHttpResponseHeaders(headers_);
    }
    HttpResponseBuilder(HttpSession* session, const BufferPtr& body):
      HttpMessageBuilder(session),
      status_code_(HttpStatusCode::kHttpOk),
      body_(body){
      InitHttpResponseHeaders(headers_);
    }
    HttpResponseBuilder(const HttpResponseBuilder& other):
      HttpMessageBuilder(other),
      status_code_(other.status_code_),
      body_(other.body_){
      InitHttpResponseHeaders(headers_);
    }
    ~HttpResponseBuilder() = default;

    void SetStatusCode(const HttpStatusCode& status_code){
      status_code_ = status_code;
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

    HttpResponsePtr Build() const{
      return std::make_shared<HttpResponse>(session_, headers_, status_code_, body_);
    }

    void operator=(const HttpResponseBuilder& other){
      HttpMessageBuilder::operator=(other);
      status_code_ = other.status_code_;
      body_ = other.body_;
    }
  };

  class HttpResponseParser : public HttpResponseBuilder{
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
      HttpResponseParser* parser = (HttpResponseParser*)p->data;
      parser->SetBody(Buffer::CopyFrom(data, len));
      return 0;
    }

    static int
    OnHeaderField(http_parser* p, const char* data, size_t len){
      HttpResponseParser* parser = (HttpResponseParser*)p->data;
      parser->SetNextHeader(std::string(data, len));
      return 0;
    }

    static int
    OnHeaderValue(http_parser* p, const char* data, size_t len){
      HttpResponseParser* parser = (HttpResponseParser*)p->data;
      parser->CreateNextHeader(std::string(data, len));
      return 0;
    }

    static int
    OnStatus(http_parser* parser, const char* data, size_t len){
      DLOG(INFO) << "status code: " << std::string(data, len);
      return 0;
    }
   public:
    HttpResponseParser(HttpSession* session):
      HttpResponseBuilder(session),
      parser_(),
      settings_(){
      parser_.data = this;
      settings_.on_status = &OnStatus;
      settings_.on_body = &OnParseBody;
      settings_.on_header_field = &OnHeaderField;
      settings_.on_header_value = &OnHeaderValue;
      http_parser_init(&parser_, HTTP_RESPONSE);
    }
    ~HttpResponseParser() = default;

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

    static inline HttpResponsePtr
    ParseResponse(HttpSession* session, const BufferPtr& buffer){
      HttpResponseParser parser(session);
      if(!parser.Parse(buffer)){
        LOG(WARNING) << "cannot parse http response.";
        return nullptr;
      }
      return parser.Build();
    }
  };

  static inline HttpResponsePtr
  NewOkResponse(HttpSession* session, const Json::String& body){
    HttpResponseBuilder builder(session);
    builder.SetStatusCode(HttpStatusCode::kHttpOk);
    builder.SetHeader(HTTP_HEADER_CONTENT_TYPE, HTTP_CONTENT_TYPE_APPLICATION_JSON);
    builder.SetHeader(HTTP_HEADER_CONTENT_LENGTH, body.GetSize());
    builder.SetBody(body);
    return builder.Build();
  }

  static inline HttpResponsePtr
  NewOkResponse(HttpSession* session, const std::string& msg="Ok"){
    Json::String body;
    Json::Writer writer(body);
    writer.StartObject();
      Json::SetField(writer, "data", msg);
    writer.EndObject();
    return NewOkResponse(session, body);
  }

  template<class T>
  static inline HttpResponsePtr
  NewOkResponse(HttpSession* session, const std::shared_ptr<T>& val){
    Json::String body;
    Json::Writer writer(body);
    writer.StartObject();
      writer.Key("data");
      val->Write(writer);
    writer.EndObject();
    return NewOkResponse(session, body);
  }

  static inline HttpResponsePtr
  NewErrorResponse(HttpSession* session, const HttpStatusCode& status_code, const std::string& msg){
    HttpResponseBuilder builder(session);

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

  static inline HttpResponsePtr
  NewInternalServerErrorResponse(HttpSession* session, const std::string& message){
    return NewErrorResponse(session, HttpStatusCode::kHttpInternalServerError, message);
  }

  static inline HttpResponsePtr
  NewInternalServerErrorResponse(HttpSession* session, const std::stringstream& message){
    return NewErrorResponse(session, HttpStatusCode::kHttpInternalServerError, message.str());
  }

  static inline HttpResponsePtr
  NewNotImplementedResponse(HttpSession* session, const std::string& msg){
    return NewErrorResponse(session, HttpStatusCode::kHttpNotImplemented, msg);
  }

  static inline HttpResponsePtr
  NewNotImplementedResponse(HttpSession* session, const std::stringstream& msg){
    return NewErrorResponse(session, HttpStatusCode::kHttpNotImplemented, msg.str());
  }

  static inline HttpResponsePtr
  NewNoContentResponse(HttpSession* session, const std::string& msg){
    return NewErrorResponse(session, HttpStatusCode::kHttpNoContent, msg);
  }

  static inline HttpResponsePtr
  NewNoContentResponse(HttpSession* session, const std::stringstream& msg){
    return NewErrorResponse(session, HttpStatusCode::kHttpNoContent, msg.str());
  }

  static inline HttpResponsePtr
  NewNoContentResponse(HttpSession* session, const Hash& hash){
    std::stringstream ss;
    ss << "Cannot find: " << hash;
    return NewNoContentResponse(session, ss);
  }

  static inline HttpResponsePtr
  NewNotFoundResponse(HttpSession* session, const std::string& msg){
    return NewErrorResponse(session, HttpStatusCode::kHttpNotFound, msg);
  }

  static inline HttpResponsePtr
  NewNotFoundResponse(HttpSession* session, const std::stringstream& msg){
    return NewErrorResponse(session, HttpStatusCode::kHttpNotFound, msg.str());
  }

  static inline HttpResponsePtr
  NewNotFoundResponse(HttpSession* session, const Hash& hash){
    std::stringstream ss;
    ss << "Not Found: " << hash;
    return NewNotFoundResponse(session, ss);
  }

  static inline HttpResponsePtr
  NewNotSupportedResponse(HttpSession* session, const std::string& msg){
    return NewErrorResponse(session, HttpStatusCode::kHttpNotImplemented, msg);
  }

  static inline HttpResponsePtr
  NewNotSupportedResponse(HttpSession* session, const std::stringstream& msg){
    return NewErrorResponse(session, HttpStatusCode::kHttpNotImplemented, msg.str());
  }
}

#endif //TOKEN_INCLUDE_HTTP_RESPONSE_H