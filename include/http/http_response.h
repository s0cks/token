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
    std::string body_;
    HttpStatusCode status_code_;
    HttpHeadersMap headers_;

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
      return buff->PutString("\r\n");
    }

    static int
    OnParseBody(http_parser* parser, const char* data, size_t len){
      HttpResponse* response = (HttpResponse*)parser->data;
      response->body_ = std::string(data, len);
      return 0;
    }
   public:
    HttpResponse(HttpSession* session, const HttpStatusCode& code):
      HttpMessage(session),
      body_(),
      status_code_(code),
      headers_(){
      InitHttpResponseHeaders(headers_);
    }
    HttpResponse(HttpSession* session, const HttpHeadersMap& headers, const std::string& body):
      HttpMessage(session),
      body_(body),
      headers_(headers){}
    virtual ~HttpResponse() = default;

    DEFINE_HTTP_MESSAGE(Response);

    HttpStatusCode GetStatusCode() const{
      return status_code_;
    }

    int64_t GetContentLength() const{
      std::string length = GetHeader(HTTP_HEADER_CONTENT_LENGTH);
      return atoll(length.data());
    }

    int64_t GetBufferSize() const{
      int64_t size = 0;
      size += GetHttpStatusLine(GetStatusCode()).length();
      for(auto& it : headers_)
        size += GetHttpHeaderLine(it.first, it.second).length();
      size += sizeof('\r');
      size += sizeof('\n');
      size += GetContentLength();
      return size;
    }

    std::string GetBody() const{
      return body_;
    }

    std::string ToString() const{
      return GetBody();//TODO: implement
    }
  };

  class HttpResponseBuilder : public HttpMessageBuilder<HttpResponse>{
   private:
    HttpStatusCode status_;
    std::string body_;
   public:
    HttpResponseBuilder(HttpSession* session):
      HttpMessageBuilder(session),
      status_(HttpStatusCode::kHttpOk),
      body_(){}
    ~HttpResponseBuilder() = default;

    void SetStatus(const HttpStatusCode& status_code){
      status_ = status_code;
    }

    void SetBody(const std::string& val){
      body_ = val;
    }

    HttpResponsePtr Build() const{
      return std::make_shared<HttpResponse>(session_, headers_, body_);
    }
  };

  class HttpResponseParser : public HttpResponseBuilder{
   private:
    http_parser parser_;
    http_parser_settings settings_;
    std::string header_;

    static int
    OnParseBody(http_parser* parser, const char* data, size_t len){
      HttpResponseParser* response_parser = (HttpResponseParser*)parser->data;
      response_parser->SetBody(std::string(data, len));
      return 0; //TODO: cleanup
    }

    static int
    OnHeaderField(http_parser* parser, const char* data, size_t len){
      HttpResponseParser* response_parser = (HttpResponseParser*)parser->data;
      response_parser->header_ = std::string(data, len);
      return 0;
    }

    static int
    OnHeaderValue(http_parser* parser, const char* data, size_t len){
      HttpResponseParser* response_parser = (HttpResponseParser*)parser->data;
      std::string value(data, len);
      response_parser->SetHeader(response_parser->header_, std::string(data, len));
      return 0;
    }

    static int
    OnStatus(http_parser* parser, const char* data, size_t len){
#ifdef TOKEN_DEBUG
      LOG(INFO) << "status code: " << std::string(data, len);
#endif//TOKEN_DEBUG
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

  class HttpFileResponse : public HttpResponse{
   private:
    std::string filename_;
    BufferPtr body_;
   protected:
    bool Write(const BufferPtr& buff) const{
      HttpResponse::Write(buff);
      buff->PutBytes(body_);
      return true;
    }
   public:
    HttpFileResponse(HttpSession* session,
      const HttpStatusCode& status_code,
      const std::string& filename,
      const std::string& content_type = HTTP_CONTENT_TYPE_TEXT_PLAIN):
      HttpResponse(session, status_code),
      filename_(filename),
      body_(){
      body_ = Buffer::FromFile(filename);
      SetHeader(HTTP_HEADER_CONTENT_LENGTH, body_->GetWrittenBytes());
      SetHeader(HTTP_HEADER_CONTENT_TYPE, content_type);
    }
    ~HttpFileResponse() = default;

    std::string ToString() const{
      std::stringstream ss;
      ss << "HttpFileResponse(" << filename_ << ")";
      return ss.str();
    }

    static inline HttpResponsePtr
    NewInstance(HttpSession* session,
      const HttpStatusCode& status_code,
      const std::string& filename,
      const std::string& content_type = HTTP_CONTENT_TYPE_TEXT_PLAIN){
      return std::make_shared<HttpFileResponse>(session, status_code, filename, content_type);
    }
  };

  class HttpJsonResponse : public HttpResponse{
   private:
    Json::String body_;
   protected:
    bool Write(const BufferPtr& buffer) const{
      if(!HttpResponse::Write(buffer)){
        LOG(WARNING) << "couldn't write http message status & headers.";
        return false;
      }

      if(!buffer->PutBytes((uint8_t*)body_.GetString(), body_.GetSize())){
        LOG(WARNING) << "couldn't write http message body.";
        return false;
      }
      return true;
    }
   public:
    HttpJsonResponse(HttpSession* session, const HttpStatusCode& status_code):
      HttpResponse(session, status_code),
      body_(){}
    ~HttpJsonResponse() = default;

    Json::String& GetBody(){
      return body_;
    }

    std::string ToString() const{
      std::stringstream ss;
      ss << "HttpJsonResponse(";

      ss << "headers={";
      for(auto& it : headers_){
        ss << it.first << ": " << it.second << ",";
      }
      ss << "}, ";

      ss << "body=" << body_.GetString();
      ss << ")";
      return ss.str();
    }
  };

  class HttpBinaryResponse : public HttpResponse{
   private:
    const BufferPtr data_;
   protected:
    bool Write(const BufferPtr& buff) const{
      return HttpResponse::Write(buff)
          && buff->PutBytes(data_);
    }
   public:
    HttpBinaryResponse(HttpSession* session, const HttpStatusCode& status_code, const std::string& content_type, const BufferPtr& data):
      HttpResponse(session, status_code),
      data_(data){
      SetHeader(HTTP_HEADER_CONTENT_TYPE, content_type);
      SetHeader(HTTP_HEADER_CONTENT_LENGTH, data->GetWrittenBytes());
    }
    ~HttpBinaryResponse() = default;

    std::string ToString() const{
      std::stringstream ss;
      ss << "HttpBinaryResponse(";

      ss << "headers={";
      for(auto& it : headers_){
        ss << it.first << ": " << it.second << ",";
      }
      ss << "}, ";

      ss << "body=" << data_->GetBufferSize() << " bytes,";

      ss << ")";
      return ss.str();
    }

    static inline HttpResponsePtr
    NewInstance(HttpSession* session, const HttpStatusCode& status_code, const std::string& content_type, const BufferPtr& data){
      return std::make_shared<HttpBinaryResponse>(session, status_code, content_type, data);
    }
  };

  static inline HttpResponsePtr
  NewOkResponse(HttpSession* session, const HttpStatusCode& status_code=HttpStatusCode::kHttpOk, const std::string& msg="Ok"){
    HttpJsonResponsePtr response = std::make_shared<HttpJsonResponse>(session, status_code);

    Json::String& body = response->GetBody();
    Json::Writer writer(body);
    writer.StartObject();
      Json::SetField(writer, "data", msg);
    writer.EndObject();

    response->SetHeader("Content-Type", HTTP_CONTENT_TYPE_APPLICATION_JSON);
    response->SetHeader("Content-Length", body.GetSize());
    return std::static_pointer_cast<HttpResponse>(response);
  }

  static inline HttpResponsePtr
  NewOkResponse(HttpSession* session, const BlockPtr& blk){
    std::shared_ptr<HttpJsonResponse> response = std::make_shared<HttpJsonResponse>(session, HttpStatusCode::kHttpOk);

    Json::String& body = response->GetBody();
    Json::Writer writer(body);
    writer.StartObject();
      writer.Key("data");
      blk->Write(writer);
    writer.EndObject();
    response->SetHeader("Content-Type", HTTP_CONTENT_TYPE_APPLICATION_JSON);
    response->SetHeader("Content-Length", body.GetSize());
    response->SetHeader("Connection", "close");
    response->SetHeader("Content-Encoding", "identity");
    return std::static_pointer_cast<HttpResponse>(response);
  }

  static inline HttpResponsePtr
  NewOkResponse(HttpSession* session, const TransactionPtr& tx){
    HttpJsonResponsePtr response = std::make_shared<HttpJsonResponse>(session, HttpStatusCode::kHttpOk);

    Json::String& body = response->GetBody();
    Json::Writer writer(body);
    writer.StartObject();
      writer.Key("data");
      tx->Write(writer);
    writer.EndObject();

    response->SetHeader("Content-Type", HTTP_CONTENT_TYPE_APPLICATION_JSON);
    response->SetHeader("Content-Length", body.GetSize());
    return std::static_pointer_cast<HttpResponse>(response);
  }

  static inline HttpResponsePtr
  NewOkResponse(HttpSession* session, const UnclaimedTransactionPtr& utxo){
    HttpJsonResponsePtr response = std::make_shared<HttpJsonResponse>(session, HttpStatusCode::kHttpOk);

    Json::String& body = response->GetBody();
    Json::Writer writer(body);
    writer.StartObject();
      writer.Key("data");
      utxo->Write(writer);
    writer.EndObject();

    response->SetHeader("Content-Type", HTTP_CONTENT_TYPE_APPLICATION_JSON);
    response->SetHeader("Content-Length", body.GetSize());
    return std::static_pointer_cast<HttpResponse>(response);
  }

  static inline HttpResponsePtr
  NewErrorResponse(HttpSession* session, const HttpStatusCode& status_code, const std::string& msg){
    HttpJsonResponsePtr response = std::make_shared<HttpJsonResponse>(session, status_code);

    Json::String& body = response->GetBody();
    Json::Writer writer(body);
    writer.StartObject();
      Json::SetField(writer, "code", status_code);
      Json::SetField(writer, "message", msg);
    writer.EndObject();

    response->SetHeader("Content-Type", HTTP_CONTENT_TYPE_APPLICATION_JSON);
    response->SetHeader("Content-Length", body.GetSize());
    return std::static_pointer_cast<HttpResponse>(response);
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