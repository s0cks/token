#ifndef TOKEN_ALLOCATOR_H
#define TOKEN_ALLOCATOR_H

#include <iostream>
#include <vector>
#include <condition_variable>
#include "common.h"
#include "handle.h"
#include "memory_pool.h"

namespace Token{
    enum class Space{
        kStackSpace = 1,
        kNewSpace,
        kOldSpace,
        kMemoryPool
    };

    static std::ostream& operator<<(std::ostream& stream, const Space& space){
        switch(space){
            case Space::kStackSpace:
                stream << "Stack";
                return stream;
            case Space::kNewSpace:
                stream << "New";
                return stream;
            case Space::kOldSpace:
                stream << "Old";
                return stream;
            case Space::kMemoryPool:
                stream << "Memory Pool";
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
        friend class HandleBase;
        friend class HeapDumpWriter;
        friend class MemoryInformationSection;
    private:
        Allocator() = delete;

        //TODO: refactor roots again
        static Object** TrackRoot(Object* value);
        static void FreeRoot(Object** root);
        static void UntrackRoot(Object* root);

        static bool VisitRoots(WeakObjectPointerVisitor* vis);
        static void Initialize(Object* obj);
    public:
        ~Allocator(){}

        static void Initialize();
        static void MinorCollect();
        static void MajorCollect();

        static void* Allocate(size_t size);
        static MemoryRegion* GetRegion();
        static Heap* GetHeap();
        static MemoryPool* GetUnclaimedTransactionPoolMemory();
        static MemoryPool* GetTransactionPoolMemory();
        static MemoryPool* GetBlockPoolMemory();
        static size_t GetNumberOfStackSpaceObjects();
        static size_t GetStackSpaceSize();
        static void PrintHeap();
    };

    class WeakObjectPointerVisitor{
    protected:
        WeakObjectPointerVisitor() = default;
    public:
        virtual ~WeakObjectPointerVisitor() = default;
        virtual bool Visit(Object** root) = 0;
    };

    //TODO: remove GCStats
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
        GCStats(): GCStats(Space::kNewSpace){}
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

        bool IsNewSpace() const{
            return GetSpace() == Space::kNewSpace;
        }

        bool IsOldSpace() const{
            return GetSpace() == Space::kOldSpace;
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
        virtual bool Visit(Object** field) const = 0;

        template<typename T>
        bool Visit(T** field) const{
            return Visit((Object**)field);
        }
    };
}

#endif //TOKEN_ALLOCATOR_H