#ifndef TOKEN_TIMELINE_H
#define TOKEN_TIMELINE_H

#include <set>
#include "printer.h"

namespace Token{
  class TimelineEvent{
   public:
    struct TimestampComparator{
      bool operator()(const TimelineEvent& a, const TimelineEvent& b){
        return a.timestamp_ < b.timestamp_;
      }
    };

    struct NameComparator{
      bool operator()(const TimelineEvent& a, const TimelineEvent& b){
        return a.name_ < b.name_;
      }
    };
   private:
    Timepoint timestamp_;
    std::string name_;
   public:
    TimelineEvent(const std::string& name, const Timepoint& timestamp = Clock::now()):
      timestamp_(timestamp),
      name_(name){}
    TimelineEvent(const TimelineEvent& event):
      timestamp_(event.timestamp_),
      name_(event.name_){}
    ~TimelineEvent() = default;

    Timepoint GetTimestamp() const{
      return timestamp_;
    }

    std::string GetName() const{
      return name_;
    }

    void operator=(const TimelineEvent& event){
      timestamp_ = event.timestamp_;
      name_ = event.name_;
    }

    friend bool operator==(const TimelineEvent& a, const TimelineEvent& b){
      return a.GetTimestamp() == b.GetTimestamp()
             && a.GetName() == b.GetName();
    }

    friend bool operator!=(const TimelineEvent& a, const TimelineEvent& b){
      return !operator==(a, b);
    }
  };

  typedef std::set<TimelineEvent, TimelineEvent::NameComparator> TimelineEventSet;
  typedef std::vector<TimelineEvent> TimelineEventList;

  class TimelineEventVisitor;
  class Timeline{
   private:
    typedef std::chrono::steady_clock::time_point Timepoint;

    std::string name_;
    TimelineEventSet events_;
   public:
    Timeline(const std::string& name):
      name_(name),
      events_(){}
    Timeline(const Timeline& timeline):
      name_(timeline.name_),
      events_(timeline.events_){}
    ~Timeline() = default;

    std::string GetName() const{
      return name_;
    }

    TimelineEventSet& events(){
      return events_;
    }

    TimelineEventSet events() const{
      return events_;
    }

    Timepoint GetStartTime() const{
      if(events_.empty()) return Clock::now();
      return (*events_.begin()).GetTimestamp();
    }

    Timepoint GetStopTime() const{
      if(events_.empty()) return Clock::now();
      return (*events_.rbegin()).GetTimestamp();
    }

    Duration GetTotalTime() const{
      return GetStopTime() - GetStartTime();
    }

    TimelineEventSet::iterator begin(){
      return events_.begin();
    }

    TimelineEventSet::const_iterator begin() const{
      return events_.begin();
    }

    TimelineEventSet::iterator end(){
      return events_.end();
    }

    TimelineEventSet::const_iterator end() const{
      return events_.end();
    }

    void operator=(const Timeline& timeline){
      name_ = timeline.name_;
      events_ = timeline.events_;
    }

    friend Timeline& operator<<(Timeline& timeline, const std::string& name){
      timeline.events_.insert(TimelineEvent(name));
      return timeline;
    }
  };

  class TimelinePrinter : public Printer{
   public:
    TimelinePrinter(const google::LogSeverity& severity = google::INFO, const long& flags = Printer::kFlagNone):
      Printer(severity, flags){}
    ~TimelinePrinter() = default;

    bool Print(const Timeline& timeline) const{
      LOG_AT_LEVEL(GetSeverity()) << timeline.GetName();
      LOG_AT_LEVEL(GetSeverity()) << "---------------------------------------------";
      LOG_AT_LEVEL(GetSeverity()) << "  Start: "
                                  << GetTimestampFormattedReadable(timeline.GetStartTime().time_since_epoch().count());
      LOG_AT_LEVEL(GetSeverity()) << "  Stop: "
                                  << GetTimestampFormattedReadable(timeline.GetStopTime().time_since_epoch().count());
      LOG_AT_LEVEL(GetSeverity()) << "  Total Time (Seconds): " << timeline.GetTotalTime().count();

      if(IsDetailed()){
        //TODO: TimelinePrinter::Print(const Timeline&) - ordering can be weird
        LOG_AT_LEVEL(GetSeverity()) << "  Events:";

        TimelineEventList events;
        std::copy(timeline.begin(), timeline.end(), std::inserter(events, events.begin()));
        std::sort(events.begin(), events.end(), TimelineEvent::TimestampComparator());

        auto last = timeline.GetStartTime();
        for(auto& event : events){
          auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(event.GetTimestamp() - last);

          LOG_AT_LEVEL(GetSeverity()) << "    - " << event.GetName() << " (" << duration.count() << " Milliseconds)";

          last = event.GetTimestamp();
        }
      }
      LOG_AT_LEVEL(GetSeverity()) << "---------------------------------------------";
      return true;
    }

    bool operator()(const Timeline& timeline) const{
      return Print(timeline);
    }

    static inline bool
    PrintTimeline(const Timeline& timeline, const google::LogSeverity& severity = google::INFO){
      TimelinePrinter printer(severity);
      return printer(timeline);
    }
  };
}

#endif //TOKEN_TIMELINE_H