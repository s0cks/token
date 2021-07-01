#ifndef TOKEN_ELASTIC_EVENT_H
#define TOKEN_ELASTIC_EVENT_H

#ifdef TOKEN_ENABLE_EXPERIMENTAL

#include "json.h"
#include "timestamp.h"

namespace token{
  namespace elastic{
    class EventClass{
     protected:
      std::string kind_;
      std::string dataset_;
     public:
      EventClass():
        kind_(),
        dataset_(){}
      EventClass(const std::string& kind, const std::string& dataset):
        kind_(kind),
        dataset_(dataset){}
      EventClass(const EventClass& other) = default;
      ~EventClass() = default;

      std::string& kind(){
        return kind_;
      }

      std::string kind() const{
        return kind_;
      }

      std::string& dataset(){
        return dataset_;
      }

      std::string dataset() const{
        return dataset_;
      }

      EventClass& operator=(const EventClass& other) = default;

      friend std::ostream&
      operator<<(std::ostream& stream, const EventClass& val){
        stream << "EventClass(";
        stream << "kind=" << val.kind() << ", ";
        stream << "dataset=" << val.dataset();
        stream << ")";
        return stream;
      }

      friend json::Writer&
      operator<<(json::Writer& writer, const EventClass& val){
        if(!writer.StartObject()){
          DLOG(ERROR) << "cannot start EventClass json object.";
          return writer;
        }

        if(!json::SetField(writer, "kind", val.kind())){
          DLOG(ERROR) << "cannot set EventClass kind json object field.";
          return writer;
        }

        if(!json::SetField(writer, "dataset", val.dataset())){
          DLOG(ERROR) << "cannot set EventClass dataset json object field.";
          return writer;
        }

        if(!writer.EndObject()){
          DLOG(ERROR) << "cannot end EventClass json object.";
          return writer;
        }
        return writer;
      }
    };

    class Event{
     protected:
      Timestamp timestamp_;
      EventClass event_;
     public:
      Event():
        timestamp_(),
        event_(){}
      Event(const Timestamp& timestamp, const EventClass& cls):
        timestamp_(timestamp),
        event_(cls){}
      Event(const Timestamp& timestamp, const std::string& kind, const std::string& dataset):
        timestamp_(timestamp),
        event_(EventClass(kind, dataset)){}
      Event(const Event& other) = default;
      virtual ~Event() = default;

      Timestamp& timestamp(){
        return timestamp_;
      }

      Timestamp timestamp() const{
        return timestamp_;
      }

      EventClass& event_class(){
        return event_;
      }

      EventClass event_class() const{
        return event_;
      }

      Event& operator=(const Event& other) = default;

      friend std::ostream&
      operator<<(std::ostream& stream, const Event& val){
        stream << "Event(";
        stream << "timestamp=" << ToUnixTimestamp(val.timestamp()) << ", ";
        stream << "event=" << val.event_class();
        stream << ")";
        return stream;
      }

      friend json::Writer&
      operator<<(json::Writer& writer, const Event& val){
        if(!writer.StartObject()){
          DLOG(ERROR) << "cannot start Event json object.";
          return writer;
        }

        if(!writer.Key("event")){
          DLOG(ERROR) << "cannot set Event event json object field.";
          return writer;
        }
        writer << val.event_class();

        if(!writer.EndObject()){
          DLOG(ERROR) << "cannot end Event json object.";
          return writer;
        }
        return writer;
      }
    };
  }
}

#endif//TOKEN_ENABLE_EXPERIMENTAL
#endif//TOKEN_ELASTIC_EVENT_H