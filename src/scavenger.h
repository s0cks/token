#ifndef TOKEN_GCMODE_NONE
#ifndef TOKEN_SCAVENGER_H
#define TOKEN_SCAVENGER_H

#include <set>

namespace Token{
    class Event{
    public:
        struct TimestampComparator{
            bool operator()(const Event& a, const Event& b){
                return a.GetTimestamp() < b.GetTimestamp();
            }
        };

        struct NameComparator{
            bool operator()(const Event& a, const Event& b){
                return a.GetName() < b.GetName();
            }
        };
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
    };

    class ScavengerTimeline{
    private:
        std::string name_;
        std::set<Event, Event::NameComparator> events_;
    public:
        ScavengerTimeline(const std::string& name):
            name_(name),
            events_(){}
        ~ScavengerTimeline() = default;

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
    };

    static std::ostream& operator<<(std::ostream& stream, const ScavengerTimeline& timeline){
        std::vector<Event> events;
        std::copy(timeline.begin(), timeline.end(), std::inserter(events, events.begin()));
        std::sort(events.begin(), events.end(), Event::TimestampComparator());

        stream << timeline.GetName() << " Timeline:" << std::endl;
        stream << "\tStart Time: " << GetTimestampFormattedReadable(timeline.GetStartTime()) << std::endl;
        stream << "\tStop Time: " << GetTimestampFormattedReadable(timeline.GetStopTime()) << std::endl;
        stream << "\tTotal Time (Seconds): " << timeline.GetTotalTime() << std::endl;
        stream << "--------------------------------------------------" << std::endl;
        auto last = timeline.GetStartTime();
        for(auto& event : events){
            auto total_seconds = (event.GetTimestamp() - last);
            stream << "\t- " << event.GetName() << " (" << total_seconds << " seconds)" << std::endl;
        }
        stream << "--------------------------------------------------" << std::endl;
        return stream;
    }

    class ScavengerStats{
        friend class Scavenger;
    private:
        Heap* heap_;
        int64_t bytes_processed_;
        int64_t bytes_collected_;
        int64_t bytes_survived_;
        int64_t bytes_promoted_;

        int64_t num_processed_;
        int64_t num_collected_;
        int64_t num_promoted_;
        int64_t num_survived_;

        ScavengerStats(Heap* heap):
            heap_(heap),
            bytes_processed_(0),
            bytes_collected_(0),
            bytes_survived_(0),
            bytes_promoted_(0),
            num_processed_(0),
            num_collected_(0),
            num_promoted_(0),
            num_survived_(0){}
    public:
        ~ScavengerStats() = default;

        Heap* GetHeap() const{
            return heap_;
        }

        int64_t GetBytesCollected() const{
            return bytes_collected_;
        }

        int64_t GetBytesPromoted() const{
            return bytes_promoted_;
        }

        int64_t GetBytesSurvived() const{
            return bytes_survived_;
        }

        int64_t GetBytesProcessed() const{
            return bytes_processed_;
        }

        int64_t GetObjectsProcessed() const{
            return num_processed_;
        }

        int64_t GetObjectsPromoted() const{
            return num_promoted_;
        }

        int64_t GetObjectsSurvived() const{
            return num_survived_;
        }

        float GetObjectsSurvivedPercentage() const{
            return (num_processed_ / num_survived_) * 100;
        }

        int64_t GetObjectsCollected() const{
            return num_collected_;
        }

        float GetObjectsCollectedPercentage() const{
            return (num_processed_ / num_collected_) * 100;
        }
    };

    static std::ostream& operator<<(std::ostream& stream, const ScavengerStats& stats){
        stream << "Scavenger Stats (" << stats.GetHeap()->GetSpace() << "):" << std::endl;
        stream << "\tObjects Processed: " << stats.GetObjectsProcessed() << std::endl;
        stream << "\tObjects Promoted: " << stats.GetObjectsPromoted() << std::endl;
        stream << "\tObjects Surviving: " << stats.GetObjectsSurvived() << std::endl;
        stream << "\tObjects Collected: " << stats.GetObjectsCollected() << std::endl;
        stream << "\tBytes Processed: " << stats.GetBytesProcessed() << std::endl;
        stream << "\tBytes Promoted: " << stats.GetBytesPromoted() << std::endl;
        stream << "\tBytes Surviving: " << stats.GetBytesSurvived() << std::endl;
        stream << "\tBytes Collected: " << stats.GetBytesCollected() << std::endl;
        return stream;
    }

    class Scavenger : public ObjectPointerVisitor{
        friend class ObjectSweeper;
    public:
        enum Phase{
            kMarkPhase,
            kSweepPhase
        };

        friend std::ostream& operator<<(std::ostream& stream, const Phase& phase){
            switch(phase){
                case Phase::kMarkPhase:
                    stream << "Mark";
                    return stream;
                case Phase::kSweepPhase:
                    stream << "Sweep";
                    return stream;
            }
        }
    private:
        bool is_major_;
        Phase phase_;
        ScavengerTimeline timeline_;
        ScavengerStats old_stats_;
        ScavengerStats new_stats_;

        Scavenger(bool is_major):
            is_major_(is_major),
            phase_(Phase::kMarkPhase),
            timeline_(is_major ? "Major GC" : "Minor GC"),
            old_stats_(Allocator::GetOldHeap()),
            new_stats_(Allocator::GetNewHeap()){}

        Phase GetPhase() const{
            return phase_;
        }

        void SetPhase(Phase phase){
            phase_ = phase;
        }

        bool ProcessRoots();
        bool ScavengeMemory();
        bool Visit(Object* obj);
        bool FinalizeObject(Object* obj);
        bool PromoteObject(Object* obj);
        bool ScavengeObject(Object* obj);

#ifdef TOKEN_DEBUG
        void PrintNewHeap();
        void PrintOldHeap();
#endif//TOKEN_DEBUG
    public:
        ~Scavenger() = default;

        bool IsMajor() const{
            return is_major_;
        }

        ScavengerTimeline& GetTimeline(){
            return timeline_;
        }

        ScavengerStats& GetNewStats(){
            return new_stats_;
        }

        ScavengerStats& GetOldStats(){
            return old_stats_;
        }

        static bool Scavenge(bool is_major);
    };
}

#endif //TOKEN_SCAVENGER_H
#endif//TOKEN_GCMODE_NONE