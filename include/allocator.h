#ifdef TOKEN_GCMODE_DEFAULT

#ifndef TOKEN_ALLOCATOR_H
#define TOKEN_ALLOCATOR_H

#include <vector>
#include <iostream>
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
    class Object;
    class MemoryRegion;
    class ObjectPointerVisitor;
    class WeakObjectPointerVisitor;
    class Allocator{
        friend class Object;
        friend class Scavenger;
        friend class HeapDumpWriter;
    public:
        static const intptr_t kDefaultHeapSize;
        static const char* kDefaultHeapSizeAsString;
    private:
        Allocator() = delete;
        static MemoryRegion* GetRegion();
        static Heap* GetNewHeap();
        static Heap* GetOldHeap();
        static bool MinorCollect();
        static bool MajorCollect();
        static void Initialize(Object* obj); //TODO: remove
        static void* Allocate(int64_t size);
    public:
        ~Allocator(){}

        static void Initialize();
        static void* AllocateObject(int64_t size);
        static void PrintNewHeap();
        static void PrintOldHeap();
    };

    class WeakObjectPointerVisitor{
    protected:
        WeakObjectPointerVisitor() = default;
    public:
        virtual ~WeakObjectPointerVisitor() = default;
        virtual bool Visit(Object** root) = 0;

        template<typename T>
        bool Visit(T** ptr){
            return Visit((Object**)ptr);
        }
    };
}

#endif//TOKEN_ALLOCATOR_H

#endif//TOKEN_GCMODE_DEFAULT