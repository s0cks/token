#ifndef TOKEN_SCAVENGER_H
#define TOKEN_SCAVENGER_H

namespace Token{
    class Scavenger;
    class ScavengerStats{
        friend class Scavenger;
    private:
        Scavenger* parent_;
        Heap* heap_;

        intptr_t start_time_;
        intptr_t stop_time_;

        intptr_t start_size_;
        intptr_t stop_size_;

        ScavengerStats(Scavenger* scavenger, Heap* heap);
    public:
        ~ScavengerStats() = default;

        Scavenger* GetParent() const{
            return parent_;
        }

        Heap* GetHeap() const{
            return heap_;
        }

        intptr_t GetStartTime() const{
            return start_time_;
        }

        intptr_t GetStopTime() const{
            return stop_time_;
        }

        intptr_t GetTotalCollectionTimeMilliseconds() const{
            return GetStopTime() - GetStartTime();
        }

        intptr_t GetStartSize() const{
            return start_size_;
        }

        intptr_t GetStopSize() const{
            return stop_size_;
        }

        intptr_t GetTotalBytesCollected() const{
            return GetStartSize() - GetStopSize();
        }

        void CollectionFinished();
    };

    class Scavenger{
    private:
        bool is_major_;

        Scavenger(bool is_major):
            is_major_(is_major){}

        bool ScavengeMemory();
    public:
        ~Scavenger() = default;

        static bool Scavenge(bool is_major){
            Scavenger scavenger(is_major);
            return scavenger.ScavengeMemory();
        }
    };
}

#endif //TOKEN_SCAVENGER_H