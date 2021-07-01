#ifndef TOKEN_ELASTIC_CLIENT_H
#define TOKEN_ELASTIC_CLIENT_H

#ifdef TOKEN_ENABLE_EXPERIMENTAL

#include "http/http_client.h"
#include "elastic/elastic_event.h"

namespace token{
  static bool SendEvent(const NodeAddress& address, const std::string& index, const elastic::Event& event);

  template<class T>
  static inline bool
  SendEvent(const NodeAddress& address, const T& event){
    json::String doc;
    json::Writer writer(doc);
    writer << event;

    HttpClient client(address);
    http::RequestBuilder builder;
    builder.SetMethod(http::Method::kPost);
    builder.SetPath("/");
    builder.SetHeader(HTTP_HEADER_CONTENT_TYPE, HTTP_CONTENT_TYPE_APPLICATION_JSON);
    builder.SetHeader(HTTP_HEADER_CONTENT_LENGTH, doc.GetSize());
    builder.SetBody(doc);

    http::ResponsePtr response = client.Send(builder.Build());
    if(!response){
      DLOG(WARNING) << "empty response from ES.";
      return false;
    }

    DLOG(INFO) << "response from ES: " << response->ToString();
    return true;//TODO: check status code
  }
}

#endif//TOKEN_ENABLE_EXPERIMENTAL
#endif//TOKEN_ELASTIC_CLIENT_H