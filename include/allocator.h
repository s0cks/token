#ifndef TOKEN_ALLOCATOR_H
#define TOKEN_ALLOCATOR_H

#include <vector>
#include <condition_variable>
#include "common.h"

namespace Token{
    enum class Space{
        kStackSpace,
        kEdenSpace,
        kSurvivorSpace,
        kTenuredSpace,
    };

    //TODO
    // - refactor stack allocations
    // - locking for allocations + collections
    class Heap;
    class Allocator{
        friend class Object;
        friend class Scavenger;
        friend class MemoryInformationSection;
    private:
        Allocator() = delete;

        static void NotifyWeakStackReferences();
        static void UpdateStackReferences();

        static void Initialize(Object* obj);
        static void UntrackStackObject(Object* obj);
    public:
        ~Allocator(){}

        static void Initialize();
        static void* Allocate(size_t size);
        static Heap* GetEdenHeap();
        static Heap* GetSurvivorHeap();
    };

    class GCStats{
        friend class Allocator;
        friend class Scavenger;
    private:
        Space space_;
        uint8_t survived_;

        void SetSpace(Space space){
            space_ = space;
        }
    public:
        GCStats(Space space):
            space_(space),
            survived_(0){}
        GCStats(): GCStats(Space::kEdenSpace){}
        ~GCStats(){}

        uint8_t GetNumberOfCollectionsSurvived() const{
            return survived_;
        }

        Space GetSpace() const{
            return space_;
        }

        bool IsStackSpace() const{
            return GetSpace() == Space::kStackSpace;
        }

        bool InEdenSpace() const{
            return GetSpace() == Space::kEdenSpace;
        }

        bool IsSurvivorSpace() const{
            return GetSpace() == Space::kSurvivorSpace;
        }

        bool IsTenuredSpace() const{
            return GetSpace() == Space::kTenuredSpace;
        }

        GCStats& operator=(const GCStats& other){
            space_ = other.space_;
            survived_ = other.survived_;
            return (*this);
        }
    };
}

#endif //TOKEN_ALLOCATOR_H