#ifndef TOKEN_TIMELINE_H
#define TOKEN_TIMELINE_H

#include <set>
#include "common.h"

namespace Token{
    class Event{
    private:
        Timestamp timestamp_;
        std::string name_;
    public:
        Event(const std::string& name, Timestamp timestamp=GetCurrentTimestamp()):
            timestamp_(timestamp),
            name_(name){}
        Event(const Event& event):
            timestamp_(event.timestamp_),
            name_(event.name_){}
        ~Event() = default;

        Timestamp GetTimestamp() const{
            return timestamp_;
        }

        std::string GetName() const{
            return name_;
        }

        void operator=(const Event& event){
            timestamp_ = event.timestamp_;
            name_ = event.name_;
        }

        friend bool operator==(const Event& a, const Event& b){
            return a.timestamp_ == b.timestamp_
                && a.name_ == b.name_;
        }

        friend bool operator!=(const Event& a, const Event& b){
            return !operator==(a, b);
        }

        struct TimestampComparator{
            bool operator()(const Event& a, const Event& b){
                return a.GetTimestamp() < b.GetTimestamp();
            }
        };
    };

    class Timeline{
    private:
        std::string name_;
        std::set<Event, Event::TimestampComparator> events_;
    public:
        Timeline(const std::string& name):
            name_(name),
            events_(){}
        ~Timeline() = default;

        std::string GetName() const{
            return name_;
        }

        bool Push(const Event& event){
            return events_.insert(event).second;
        }

        inline bool
        Push(const std::string& name){
            return Push(Event(name));
        }

        intptr_t GetStartTime() const{
            if(events_.empty()) return 0;
            return (*events_.begin()).GetTimestamp();
        }

        intptr_t GetStopTime() const{
            if(events_.empty()) return 0;
            return (*events_.rbegin()).GetTimestamp();
        }

        intptr_t GetTotalTime() const{
            return GetStopTime() - GetStartTime();
        }

        std::set<Event>::iterator begin(){
            return events_.begin();
        }

        std::set<Event>::const_iterator begin() const{
            return events_.begin();
        }

        std::set<Event>::iterator end(){
            return events_.end();
        }

        std::set<Event>::const_iterator end() const{
            return events_.end();
        }

        bool Print(){
            LOG(INFO) << GetName();
            LOG(INFO) << "--------------------------------------------------";
            auto iter = begin();
            while(iter != end()){
                auto peek = iter;
                peek++;

                intptr_t total_time = (peek != end())
                        ? ((*peek).GetTimestamp() - (*iter).GetTimestamp())
                        : 0;
                LOG(INFO) << " - " << (*iter).GetName() << " (" << total_time << " ms)";
                iter++;
            }
            LOG(INFO) << "--------------------------------------------------";
            return true;
        }
    };
}

#endif //TOKEN_TIMELINE_H