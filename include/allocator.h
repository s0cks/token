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
        kNewHeap,
        kOldHeap,
    };

    static std::ostream& operator<<(std::ostream& stream, const Space& space){
        switch(space){
            case Space::kStackSpace:
                stream << "Stack";
                return stream;
            case Space::kNewHeap:
                stream << "New Heap";
                return stream;
            case Space::kOldHeap:
                stream << "Old Heap";
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
    class MemoryRegion;
    class ObjectPointerVisitor;
    class WeakObjectPointerVisitor;
    class Allocator{
        friend class Object;
        friend class RawObject;
        friend class Scavenger;
    private:
        Allocator() = delete;
        static void Initialize(RawObject* obj);
    public:
        ~Allocator(){}

        static void Initialize();
        static bool MinorCollect();
        static bool MajorCollect();
        static void* Allocate(size_t size);
        static MemoryRegion* GetRegion();
        static Heap* GetNewHeap();
        static Heap* GetOldHeap();
        static void PrintNewHeap();
        static void PrintOldHeap();
    };

    class WeakObjectPointerVisitor{
    protected:
        WeakObjectPointerVisitor() = default;
    public:
        virtual ~WeakObjectPointerVisitor() = default;
        virtual bool Visit(RawObject** root) = 0;

        template<typename T>
        bool Visit(T** ptr){
            return Visit((RawObject**)ptr);
        }
    };

    typedef uint64_t ObjectHeader; //TODO: Remove ObjectHeader

    class RawObject{ //TODO: Remove RawObject
        friend class Heap;
        friend class Thread;
        friend class HandleGroup;
        friend class Allocator;
        friend class Scavenger;
        friend class Semispace;
        friend class ObjectPointerPrinter;
        friend class ObjectFinalizer;
        friend class ObjectRelocator;
        friend class Marker;
        friend class LiveObjectCopier;
        friend class StackSpaceSizeCalculator;
        friend class LiveObjectReferenceUpdater;
        friend class RootObjectReferenceUpdater;
    public:
        static const intptr_t kNumberOfCollectionsRequiredForPromotion = 3;
    private:
        ObjectHeader header_;
        Space space_;
        bool marked_;
        intptr_t size_;
        intptr_t collections_survived_;
        intptr_t num_references_;
        uword ptr_;

        ObjectHeader GetAllocationHeader() const{
            return header_;
        }

        void SetAllocationHeader(ObjectHeader header){
            header_ = header;
        }

        void SetSpace(Space space){
            space_ = space;
        }

        void SetObjectSize(intptr_t size){
            size_ = size;
        }

        void IncrementReferenceCount(){
            num_references_++;
        }

        void DecrementReferenceCount(){
            num_references_--;
        }

        intptr_t GetObjectSize() const{
            return size_;
        }

        intptr_t GetReferenceCount() const{
            return num_references_;
        }

        intptr_t GetNumberOfCollectionsSurvived() const{
            return collections_survived_;
        }

        void IncrementCollectionsCounter(){
            collections_survived_++;
        }

        void SetMarked(){
            marked_ = true;
        }

        void ClearMarked(){
            marked_ = false;
        }

        bool HasStackReferences() const{
            return GetReferenceCount() > 0;
        }

        bool IsMarked() const{
            return marked_;
        }

        bool IsReadyForPromotion() const{
            return collections_survived_ >= kNumberOfCollectionsRequiredForPromotion;
        }

        Space GetSpace() const{
            return space_;
        }

        bool IsInStackSpace() const{
            return space_ == Space::kStackSpace;
        }

        bool IsNewHeap() const{
            return space_ == Space::kNewHeap;
        }
    protected:
        RawObject():
            header_(0),
            space_(),
            collections_survived_(0),
            num_references_(0),
            size_(0),
            marked_(false),
            ptr_(0){
            Allocator::Initialize(this);
        }

        virtual void NotifyWeakReferences(RawObject** field){}
        virtual bool Accept(WeakObjectPointerVisitor* vis){ return true; }

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
            //TODO: needed? if(IsInStackSpace()) Allocator::UntrackRoot(this);
        }

        size_t GetAllocatedSize() const{
            return GetObjectSize();// refactor
        }

        virtual std::string ToString() const = 0;

        static void* operator new(size_t size){
            return Allocator::Allocate(size);
        }
    };
}

#endif //TOKEN_ALLOCATOR_H