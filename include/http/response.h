#ifndef TOKEN_INCLUDE_HTTP_RESPONSE_H
#define TOKEN_INCLUDE_HTTP_RESPONSE_H

#include <memory>
#include "http/header.h"
#include "utils/json_conversion.h"

namespace Token{
  class HttpResponse;
  typedef std::shared_ptr<HttpResponse> HttpResponsePtr;

  class HttpJsonResponse;
  typedef std::shared_ptr<HttpJsonResponse> HttpJsonResponsePtr;

#define FOR_EACH_HTTP_RESPONSE(V) \
  V(Ok, 200)                      \
  V(NoContent, 204)               \
  V(NotFound, 404)                \
  V(InternalServerError, 500)     \
  V(NotImplemented, 501)

  enum HttpStatusCode{
#define DEFINE_CODE(Name, Val) k##Name = Val,
    FOR_EACH_HTTP_RESPONSE(DEFINE_CODE)
#undef DEFINE_CODE
  };

  class HttpSession;
  class HttpResponse{
    friend class HttpSession;
   protected:
    HttpSession* session_;
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
   public:
    HttpResponse(HttpSession* session, const HttpStatusCode& code):
      session_(session),
      status_code_(code),
      headers_(){
      SetHeader("Access-Control-Allow-Origin", "*");
    }
    virtual ~HttpResponse() = default;

    HttpSession* GetSession() const{
      return session_;
    }

    HttpStatusCode GetStatusCode() const{
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
      std::string length = GetHeaderValue(HTTP_HEADER_CONTENT_LENGTH);
      return atoll(length.data());
    }

    virtual std::string ToString() const{
      std::stringstream ss;

      BufferPtr buff = Buffer::NewInstance(GetBufferSize());
      if(!Write(buff)){
        ss << "N/A";
      } else{
        ss << std::string(buff->data(), buff->GetWrittenBytes());
      }
      return ss.str();
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
  };

  class HttpFileResponse : public HttpResponse{
   private:
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
      body_(){
      std::fstream fd(filename, std::ios::in | std::ios::binary);
      int64_t fsize = GetFilesize(fd);
      body_ = Buffer::NewInstance(fsize);
      if(!body_->ReadBytesFrom(fd, fsize)){
        LOG(WARNING) << "couldn't read " << fsize << " bytes from: " << filename;
        SetHeader(HTTP_HEADER_CONTENT_LENGTH, 0);
      } else{
        SetHeader(HTTP_HEADER_CONTENT_LENGTH, fsize);
      }
      SetHeader(HTTP_HEADER_CONTENT_TYPE, content_type);
    }
    ~HttpFileResponse() = default;

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
    JsonString body_;
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

    JsonString& GetBody(){
      return body_;
    }

    std::string ToString() const{
      std::stringstream ss;
      ss << GetHttpStatusLine(GetStatusCode());
      for(auto& it : headers_)
        ss << GetHttpHeaderLine(it.first, it.second);
      ss << body_.GetString();
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

    static inline HttpResponsePtr
    NewInstance(HttpSession* session, const HttpStatusCode& status_code, const std::string& content_type, const BufferPtr& data){
      return std::make_shared<HttpBinaryResponse>(session, status_code, content_type, data);
    }
  };

  static inline HttpResponsePtr
  NewOkResponse(HttpSession* session, const HttpStatusCode& status_code=HttpStatusCode::kOk, const std::string& msg="Ok"){
    HttpJsonResponsePtr response = std::make_shared<HttpJsonResponse>(session, status_code);

    JsonString& body = response->GetBody();
    JsonWriter writer(body);
    writer.StartObject();
      SetField(writer, "data", msg);
    writer.EndObject();

    response->SetHeader("Content-Type", HTTP_CONTENT_TYPE_APPLICATION_JSON);
    response->SetHeader("Content-Length", body.GetSize());
    return std::static_pointer_cast<HttpResponse>(response);
  }

  static inline HttpResponsePtr
  NewOkResponse(HttpSession* session, const BlockPtr& blk){
    std::shared_ptr<HttpJsonResponse> response = std::make_shared<HttpJsonResponse>(session, HttpStatusCode::kOk);

    JsonString& body = response->GetBody();
    JsonWriter writer(body);
    writer.StartObject();
      SetField(writer, "data", blk);
    writer.EndObject();
    response->SetHeader("Content-Type", HTTP_CONTENT_TYPE_APPLICATION_JSON);
    response->SetHeader("Content-Length", body.GetSize());
    response->SetHeader("Connection", "close");
    response->SetHeader("Content-Encoding", "identity");
    return std::static_pointer_cast<HttpResponse>(response);
  }

  static inline HttpResponsePtr
  NewOkResponse(HttpSession* session, const TransactionPtr& tx){
    HttpJsonResponsePtr response = std::make_shared<HttpJsonResponse>(session, HttpStatusCode::kOk);

    JsonString& body = response->GetBody();
    JsonWriter writer(body);
    writer.StartObject();
      SetField(writer, "data", tx);
    writer.EndObject();

    response->SetHeader("Content-Type", HTTP_CONTENT_TYPE_APPLICATION_JSON);
    response->SetHeader("Content-Length", body.GetSize());
    return std::static_pointer_cast<HttpResponse>(response);
  }

  static inline HttpResponsePtr
  NewOkResponse(HttpSession* session, const UnclaimedTransactionPtr& utxo){
    HttpJsonResponsePtr response = std::make_shared<HttpJsonResponse>(session, HttpStatusCode::kOk);

    JsonString& body = response->GetBody();
    JsonWriter writer(body);
    writer.StartObject();
      SetField(writer, "data", utxo);
    writer.EndObject();

    response->SetHeader("Content-Type", HTTP_CONTENT_TYPE_APPLICATION_JSON);
    response->SetHeader("Content-Length", body.GetSize());
    return std::static_pointer_cast<HttpResponse>(response);
  }

  static inline HttpResponsePtr
  NewErrorResponse(HttpSession* session, const HttpStatusCode& status_code, const std::string& msg){
    HttpJsonResponsePtr response = std::make_shared<HttpJsonResponse>(session, status_code);

    JsonString& body = response->GetBody();
    JsonWriter writer(body);
    writer.StartObject();
      SetField(writer, "code", status_code);
      SetField(writer, "message", msg);
    writer.EndObject();

    response->SetHeader("Content-Type", HTTP_CONTENT_TYPE_APPLICATION_JSON);
    response->SetHeader("Content-Length", body.GetSize());
    return std::static_pointer_cast<HttpResponse>(response);
  }

  static inline HttpResponsePtr
  NewInternalServerErrorResponse(HttpSession* session, const std::string& message){
    return NewErrorResponse(session, HttpStatusCode::kInternalServerError, message);
  }

  static inline HttpResponsePtr
  NewInternalServerErrorResponse(HttpSession* session, const std::stringstream& message){
    return NewErrorResponse(session, HttpStatusCode::kInternalServerError, message.str());
  }

  static inline HttpResponsePtr
  NewNotImplementedResponse(HttpSession* session, const std::string& msg){
    return NewErrorResponse(session, HttpStatusCode::kNotImplemented, msg);
  }

  static inline HttpResponsePtr
  NewNotImplementedResponse(HttpSession* session, const std::stringstream& msg){
    return NewErrorResponse(session, HttpStatusCode::kNotImplemented, msg.str());
  }

  static inline HttpResponsePtr
  NewNoContentResponse(HttpSession* session, const std::string& msg){
    return NewErrorResponse(session, HttpStatusCode::kNoContent, msg);
  }

  static inline HttpResponsePtr
  NewNoContentResponse(HttpSession* session, const std::stringstream& msg){
    return NewErrorResponse(session, HttpStatusCode::kNoContent, msg.str());
  }

  static inline HttpResponsePtr
  NewNoContentResponse(HttpSession* session, const Hash& hash){
    std::stringstream ss;
    ss << "Cannot find: " << hash;
    return NewNoContentResponse(session, ss);
  }

  static inline HttpResponsePtr
  NewNotFoundResponse(HttpSession* session, const std::string& msg){
    return NewErrorResponse(session, HttpStatusCode::kNotFound, msg);
  }

  static inline HttpResponsePtr
  NewNotFoundResponse(HttpSession* session, const std::stringstream& msg){
    return NewErrorResponse(session, HttpStatusCode::kNotFound, msg.str());
  }

  static inline HttpResponsePtr
  NewNotFoundResponse(HttpSession* session, const Hash& hash){
    std::stringstream ss;
    ss << "Not Found: " << hash;
    return NewNotFoundResponse(session, ss);
  }

  static inline HttpResponsePtr
  NewNotSupportedResponse(HttpSession* session, const std::string& msg){
    return NewErrorResponse(session, HttpStatusCode::kNotImplemented, msg);
  }

  static inline HttpResponsePtr
  NewNotSupportedResponse(HttpSession* session, const std::stringstream& msg){
    return NewErrorResponse(session, HttpStatusCode::kNotImplemented, msg.str());
  }
}

#endif //TOKEN_INCLUDE_HTTP_RESPONSE_H