#ifndef TOKEN_ELASTIC_EVENT_H
#define TOKEN_ELASTIC_EVENT_H

#ifdef TOKEN_ENABLE_EXPERIMENTAL

#include "json.h"
#include "timestamp.h"

namespace token{
  namespace elastic{
    class Event{
     protected:
      Timestamp timestamp_;
      std::string kind_;
      std::string dataset_;
      std::string action_;

      inline bool
      WriteTimestamp(Json::Writer& writer) const{
        return Json::SetField(writer, "@timestamp", timestamp_);
      }

      inline bool
      WriteEventFields(Json::Writer& writer) const{
        if(!writer.Key("event")){
          LOG(ERROR) << "couldn't write json key";
          return false;
        }

        if(!writer.StartObject()){
          LOG(ERROR) << "couldn't start json object";
          return false;
        }

        LOG_IF(ERROR, !Json::SetField(writer, "kind", kind_));
        LOG_IF(ERROR, !Json::SetField(writer, "dataset", dataset_));

        if(!writer.EndObject()){
          LOG(ERROR) << "couldn't end json object";
          return false;
        }
        return true;
      }
     public:
      Event(const Timestamp& timestamp,
            std::string kind,
            std::string dataset):
            timestamp_(timestamp),
            kind_(std::move(kind)),
            dataset_(std::move(dataset)){}
      explicit Event(const Event& other):
        timestamp_(other.timestamp_),
        kind_(other.kind_),
        dataset_(other.dataset_){}
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

      std::string GetAction() const{
        return action_;
      }

      virtual bool Write(Json::Writer& writer) const{
        if(!writer.StartObject())
          return false;
        if(!WriteTimestamp(writer))
          return false;
        if(!WriteEventFields(writer))
          return false;
        if(!writer.EndObject())
          return false;
        return true;
      }

      std::string ToString() const{
        Json::String val;
        Json::Writer writer(val);
        if(!Write(writer)){
          LOG(ERROR) << "cannot serialize event to json";
          return "";
        }
        return std::string(val.GetString(), val.GetSize());
      }

      friend std::ostream& operator<<(std::ostream& stream, const Event& event){
        return stream << event.ToString();
      }
    };
  }
}

#endif//TOKEN_ENABLE_EXPERIMENTAL
#endif//TOKEN_ELASTIC_EVENT_H