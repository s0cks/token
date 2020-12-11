#ifndef TOKEN_TIMELINE_H
#define TOKEN_TIMELINE_H

#include <set>
#include "common.h"

namespace Token{
    class TimelineEvent{
    public:
        struct TimestampComparator{
            bool operator()(const TimelineEvent& a, const TimelineEvent& b){
                return a.timestamp_ < b.timestamp_;
            }
        };
    private:
        Timestamp timestamp_;
        std::string name_;
    public:
        TimelineEvent(const std::string& name, const Timestamp timestamp=GetCurrentTimestamp()):
            timestamp_(timestamp),
            name_(name){}
        TimelineEvent(const TimelineEvent& event):
            timestamp_(event.timestamp_),
            name_(event.name_){}
        ~TimelineEvent() = default;

        Timestamp GetTimestamp() const{
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

    typedef std::set<TimelineEvent, TimelineEvent::TimestampComparator> TimelineEvents;

    class TimelineEventVisitor;
    class Timeline{
    private:
        std::string name_;
        TimelineEvents events_;
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

        TimelineEvents& events(){
            return events_;
        }

        TimelineEvents events() const{
            return events_;
        }

        int64_t GetStartTime() const{
            if(events_.empty()) return 0;
            return (*events_.begin()).GetTimestamp();
        }

        int64_t GetStopTime() const{
            if(events_.empty()) return 0;
            return (*events_.rbegin()).GetTimestamp();
        }

        int64_t GetTotalTime() const{
            return GetStopTime() - GetStartTime();
        }

        TimelineEvents::iterator events_begin(){
            return events_.begin();
        }

        TimelineEvents::const_iterator events_begin() const{
            return events_.begin();
        }

        TimelineEvents::iterator events_end(){
            return events_.end();
        }

        TimelineEvents::const_iterator events_end() const{
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

        friend std::ostream& operator<<(std::ostream& stream, const Timeline& timeline){
            stream << timeline.GetName() << " Timeline:" << std::endl;
            stream << "\tStart Time: " << GetTimestampFormattedReadable(timeline.GetStartTime()) << std::endl;
            stream << "\tStop Time: " << GetTimestampFormattedReadable(timeline.GetStopTime()) << std::endl;
            stream << "\tTotal Time (Seconds): " << timeline.GetTotalTime() << std::endl;
            stream << "--------------------------------------------------" << std::endl;
            auto last = timeline.GetStartTime();
            for(auto& event : timeline.events()){
                auto total_seconds = (event.GetTimestamp() - last);
                stream << "\t- " << event.GetName() << " (" << total_seconds << " seconds)" << std::endl;
            }
            stream << "--------------------------------------------------" << std::endl;
            return stream;
        }
    };
}

#endif //TOKEN_TIMELINE_H