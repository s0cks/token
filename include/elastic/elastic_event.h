#ifndef TOKEN_ELASTIC_EVENT_H
#define TOKEN_ELASTIC_EVENT_H

#ifdef TOKEN_ENABLE_EXPERIMENTAL

#include "timestamp.h"

namespace token{
  namespace elastic{
    class Event{
     protected:
      Timestamp timestamp_;
      std::string kind_;
      std::string dataset_;
     public:
      Event(const Timestamp& timestamp,
            std::string kind,
            std::string dataset):
        timestamp_(timestamp),
        kind_(kind),
        dataset_(dataset){}
      explicit Event(const Event& other) = default;
      virtual ~Event() = default;

      Timestamp GetTimestamp() const{
        return timestamp_;
      }

      std::string GetKind() const{
        return kind_;
      }

      std::string GetDataset() const{
        return dataset_;
      }

      std::string ToString() const{
        json::String doc;
        json::Writer writer(doc);
        if(!json::ToJson(doc, (*this)))
          return "{}";
        return std::string(doc.GetString(), doc.GetSize());
      }

      Event& operator=(const Event& other) = default;

      friend std::ostream& operator<<(std::ostream& stream, const Event& event){
        return stream << event.ToString();
      }
    };
  }
}

#endif//TOKEN_ENABLE_EXPERIMENTAL
#endif//TOKEN_ELASTIC_EVENT_H