#ifndef TOKEN_ELASTIC_CLIENT_H
#define TOKEN_ELASTIC_CLIENT_H

#ifdef TOKEN_ENABLE_ELASTICSEARCH

#include "http/http_client.h"
#include "elastic/elastic_event.h"

namespace token{
  static bool SendEvent(const NodeAddress& address, const std::string& index, const elastic::Event& event);

  template<class T>
  static inline bool
  SendEvent(const NodeAddress& address, const std::string& index, const T& event){
    Json::String body;
    Json::Writer writer(body);
    if(!event.Write(writer)){
#ifdef TOKEN_DEBUG
      LOG(WARNING) << "cannot write event " << event.ToString() << " to json.";
#endif//TOKEN_DEBUG
      return false;
    }

    HttpClient client(address);
    HttpRequestBuilder builder(nullptr);
    builder.SetMethod(HttpMethod::kPost);
    builder.SetPath("/" + index + "/_doc?pretty");
    builder.SetHeader(HTTP_HEADER_CONTENT_TYPE, HTTP_CONTENT_TYPE_APPLICATION_JSON);
    builder.SetHeader(HTTP_HEADER_CONTENT_LENGTH, body.GetSize());
    builder.SetBody(body);

    HttpResponsePtr response = client.Send(builder.Build());
    if(!response){
#ifdef TOKEN_DEBUG
      LOG(WARNING) << "empty response from ES service.";
#endif//TOKEN_DEBUG
      return false;
    }

#ifdef TOKEN_DEBUG
    LOG(INFO) << "response from ES: " << response->ToString();
#endif//TOKEN_DEBUG
    return true;//TODO: check status code
  }
}

#endif//TOKEN_ENABLE_ELASTICSEARCH
#endif//TOKEN_ELASTIC_CLIENT_H