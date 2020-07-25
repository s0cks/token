#ifndef TOKEN_ALLOCATOR_H
#define TOKEN_ALLOCATOR_H

#include <iostream>
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

    static std::ostream& operator<<(std::ostream& stream, const Space& space){
        switch(space){
            case Space::kStackSpace:
                stream << "Stack";
                return stream;
            case Space::kEdenSpace:
                stream << "Eden";
                return stream;
            case Space::kSurvivorSpace:
                stream << "Survivor";
                return stream;
            case Space::kTenuredSpace:
                stream << "Tenured";
                return stream;
            default:
                stream << "<Unknown Space>";
                return stream;
        }
    }

    //TODO
    // - refactor stack allocations
    // - locking for allocations + collections
    class ObjectPointerVisitor;
    class RootObjectPointerVisitor;
    class Heap;
    class Allocator{
        friend class Object;
        friend class Scavenger;
        friend class HandleBase;
        friend class MemoryInformationSection;
    private:
        Allocator() = delete;

        //TODO: refactor roots again
        static Object** AllocateRoot(Object* obj);
        static void FreeRoot(Object** obj);
        static void UntrackRoot(Object* obj);

        static void VisitRoots(RootObjectPointerVisitor* vis);
        static void Initialize(Object* obj);
    public:
        ~Allocator(){}

        static void Initialize();
        static void MinorCollect();
        static void MajorCollect();
        static void* Allocate(size_t size);
        static Heap* GetEdenHeap();
        static Heap* GetSurvivorHeap();
    };

    class RootObjectPointerVisitor{
    protected:
        RootObjectPointerVisitor() = default;
    public:
        virtual ~RootObjectPointerVisitor() = default;
        virtual bool Visit(Object** root) = 0;
    };

    class GCStats{
        friend class Allocator;
        friend class Scavenger;
    public:
        static const size_t kNumberOfCollectionsRequiredForPromotion = 3;
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