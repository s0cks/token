#ifndef TOKEN_ALLOCATOR_H
#define TOKEN_ALLOCATOR_H

#include <iostream>
#include <vector>
#include <condition_variable>
#include "common.h"
#include "handle.h"

namespace Token{
    enum class Space{
        kStackSpace = 1,
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

    enum class Color{
        kWhite=1,
        kGray,
        kBlack,
        kFree=kWhite,
        kMarked=kBlack
    };

    static std::ostream& operator<<(std::ostream& stream, const Color color){
        switch(color){
            case Color::kWhite:
                stream << "White";
                break;
            case Color::kGray:
                stream << "Gray";
                break;
            case Color::kBlack:
                stream << "Black";
                break;
            default:
                stream << "Unknown";
                break;
        }
        return stream;
    }

    class Heap;
    class RawObject;
    class ObjectPointerVisitor;
    class WeakObjectPointerVisitor;
    class Allocator{
        friend class RawObject;
        friend class Object;
        friend class Scavenger;
        friend class HandleBase;
        friend class MemoryInformationSection;
    private:
        Allocator() = delete;

        //TODO: refactor roots again
        static RawObject** TrackRoot(RawObject* value);
        static void FreeRoot(RawObject** root);
        static void UntrackRoot(RawObject* root);

        static void VisitRoots(WeakObjectPointerVisitor* vis);
        static void Initialize(RawObject* obj);
    public:
        ~Allocator(){}

        static void Initialize();
        static void MinorCollect();
        static void MajorCollect();
        static void* Allocate(size_t size);
        static Heap* GetEdenHeap();
        static Heap* GetSurvivorHeap();
    };

    class WeakObjectPointerVisitor{
    protected:
        WeakObjectPointerVisitor() = default;
    public:
        virtual ~WeakObjectPointerVisitor() = default;
        virtual bool Visit(RawObject** root) = 0;
    };

    class GCStats{
        friend class Allocator;
        friend class Scavenger;
        friend class RawObject;
    public:
        static const size_t kNumberOfCollectionsRequiredForPromotion = 3;
    private:
        Space space_;
        Color color_;
        size_t object_size_;
        size_t num_collections_;
        size_t num_references_;
    public:
        GCStats(Space space):
            space_(space),
            color_(Color::kWhite),
            object_size_(0),
            num_collections_(0),
            num_references_(0){}
        GCStats(): GCStats(Space::kEdenSpace){}
        ~GCStats(){}

        size_t GetNumberOfCollectionsSurvived() const{
            return num_collections_;
        }

        size_t GetNumberOfReferences() const{
            return num_references_;
        }

        size_t GetSize() const{
            return object_size_;
        }

        Color GetColor() const{
            return color_;
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
            num_collections_ = other.num_collections_;
            num_references_ = other.num_references_;
            return (*this);
        }
    };

    class WeakReferenceVisitor{
    protected:
        WeakReferenceVisitor() = default;
    public:
        virtual ~WeakReferenceVisitor() = default;
        virtual bool Visit(RawObject** field) const = 0;

        template<typename T>
        bool Visit(T** field) const{
            return Visit((RawObject**)field);
        }
    };

    typedef uint64_t ObjectHeader;

    class RawObject{
        friend class UpdateIterator;

        friend class Allocator;
        friend class Scavenger;
        friend class Heap;
        friend class Semispace;
        friend class RootPage;
        friend class ObjectFinalizer;
        friend class ObjectRelocator;
        friend class ObjectPromoter;
        friend class LiveObjectMarker;
        friend class LiveObjectCopier;
        friend class LiveObjectReferenceUpdater;
        friend class RootObjectReferenceUpdater;
    private:
        ObjectHeader header_;
        GCStats stats_;
        uword ptr_;

        ObjectHeader GetAllocationHeader() const{
            return header_;
        }

        void SetAllocationHeader(ObjectHeader header){
            header_ = header;
        }

        void SetSpace(Space space);
        void SetColor(Color color);
        void SetObjectSize(size_t size);
        void IncrementReferenceCount();
        void DecrementReferenceCount();
        Color GetColor() const;
        size_t GetObjectSize() const;
        size_t GetReferenceCount() const;

        size_t GetAllocatedSize() const{
            return GetObjectSize();// refactor
        }

        bool HasStackReferences() const{
            return GetReferenceCount() > 0;
        }

        bool IsGarbage() const{
            return GetColor() == Color::kFree;
        }

        bool IsMarked() const{
            return GetColor() == Color::kMarked;
        }

        bool IsReadyForPromotion() const{
            return stats_.GetNumberOfCollectionsSurvived() >= GCStats::kNumberOfCollectionsRequiredForPromotion;
        }

        Space GetSpace() const{
            return stats_.GetSpace();
        }

        bool IsInStackSpace() const{
            return stats_.GetSpace() == Space::kStackSpace;
        }

        bool IsInEdenSpace() const{
            return stats_.GetSpace() == Space::kEdenSpace;
        }

        bool IsInSurvivorSpace() const{
            return stats_.GetSpace() == Space::kSurvivorSpace;
        }

        bool IsInTenuredSpace() const{
            return stats_.GetSpace() == Space::kTenuredSpace;
        }
    protected:
        RawObject():
            header_(0),
            stats_(),
            ptr_(0){
            Allocator::Initialize(this);
        }

        virtual void NotifyWeakReferences(RawObject** field){}
        virtual void Accept(WeakReferenceVisitor* vis){}

        void WriteBarrier(RawObject** slot, RawObject* data){
            if(data) data->IncrementReferenceCount();
            if((*slot))(*slot)->DecrementReferenceCount();
            (*slot) = data;
        }

        template<typename T, typename U>
        void WriteBarrier(T** slot, U* data){
            WriteBarrier((RawObject**)slot, (RawObject*)data);
        }

        template<typename T>
        void WriteBarrier(T** slot, const Handle<T>& handle){
            WriteBarrier((RawObject**)slot, (RawObject*)handle);
        }
    public:
        virtual ~RawObject(){
            if(IsInStackSpace()) Allocator::UntrackRoot(this);
        }

        virtual std::string ToString() const = 0;

        static void* operator new(size_t size){
            return Allocator::Allocate(size);
        }

        static void* operator new[](size_t) = delete;
        static void operator delete(void*){}
    };
}

#endif //TOKEN_ALLOCATOR_H