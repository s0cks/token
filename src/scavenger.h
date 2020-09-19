#ifndef TOKEN_SCAVENGER_H
#define TOKEN_SCAVENGER_H

namespace Token{
    class ScavengerStats{
        friend class Scavenger;
    private:
        Heap* heap_;

        intptr_t start_size_;
        intptr_t stop_size_;

        ScavengerStats(Heap* heap);
    public:
        ~ScavengerStats() = default;

        Heap* GetHeap() const{
            return heap_;
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

    class Scavenger : public ObjectPointerVisitor{
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
        std::vector<uword> work_;
        Phase phase_;

        Scavenger():
                work_(),
                phase_(Phase::kMarkPhase){}

        bool HasWork() const{
            return !work_.empty();
        }

        Phase GetPhase() const{
            return phase_;
        }

        void SetPhase(Phase phase){
            phase_ = phase;
        }

        bool ProcessRoots();
        bool ScavengeMemory();
        bool NotifyReferences();
        bool Visit(Object* obj);
        bool FinalizeObject(Object* obj);
        bool PromoteObject(Object* obj);
        bool ScavengeObject(Object* obj);
    public:
        ~Scavenger() = default;

        static bool Scavenge(bool is_major);
    };
}

#endif //TOKEN_SCAVENGER_H