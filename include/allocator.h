#ifndef TOKEN_ALLOCATOR_H
#define TOKEN_ALLOCATOR_H

#include <cstdlib>
#include <cstdint>

#define GC_MAX_REFS 65536

namespace Token{
    class HeapVisitor;

    class AllocatorObject{
    public:
        AllocatorObject(){}
        virtual ~AllocatorObject(){}

        virtual void* GetRawChunk(){
            return nullptr;
        }

        virtual std::string ToString(){
            return "AllocatorObject()";
        }
    };

    class Allocator{
    public:
        static const uint64_t kMinChunkSize = 128;

        typedef uint8_t Byte;

        enum class Color{
            kFree = 0,
            kWhite,
            kBlack,
            kGray,
            kMarked = kWhite,
        };
    private:
        uint64_t minor_size_;
        uint64_t major_size_;
        Byte* minor_;
        Byte* major_;
        int free_chunk_;
        int refs_count_;
        void** backpatch_;
        void* references_[GC_MAX_REFS];

        inline int
        GetFreeChunk() const{
            return free_chunk_;
        }

        inline int
        GetNumberOfReferences() const{
            return refs_count_;
        }

        void SetReference(int idx, void* ref);
        void MarkChunk(Byte* ch);

        void BackpatchReferences();
        void BackpatchChunk(Byte* ch);
        void CopyMinorHeap();
        void MarkMinor();

        Byte* FindMajorChunk(Byte* ptr);
        void DarkenChunk(Byte* ptr);
        void DarkenRoots();
        void MarkMajorChunk(Byte* ch);
        void DarkenMajor();

        void VisitMinor(HeapVisitor* vis);
        void VisitMajor(HeapVisitor* vis);
        void* MinorAlloc(size_t size);
        void* MajorAlloc(size_t size);
        static Allocator* GetInstance();

        Allocator();

        friend class HeapPrinter;
    public:
        ~Allocator();

        static uint64_t GetMinorHeapSize();
        static uint64_t GetMajorHeapSize();

        static bool Initialize(uint64_t minheap_size, uint64_t maxheap_size);
        static void AddReference(void* ref);
        static void RemoveReference(void* ref);
        static void CollectMinor();
        static void CollectMajor();
        static void PrintMinorHeap();
        static void PrintMajorHeap();
        static void* Allocate(size_t size);
    };

    class HeapVisitor{
    public:
        HeapVisitor(){}
        virtual ~HeapVisitor(){}

        virtual bool VisitStart() = 0;
        virtual bool VisitEnd() = 0;
        virtual bool VisitChunk(int chunk, size_t size, void* ptr) = 0;
    };

    class HeapPrinter : public HeapVisitor{
    public:
        static const int BANNER_SIZE = 64;

        enum HeapSpace{
            kEden,
            kSurvivor
        };
    private:
        HeapSpace space_;
    public:
        HeapPrinter(HeapSpace space):
            space_(space){}
        ~HeapPrinter(){}

        bool VisitStart();
        bool VisitEnd();
        bool VisitChunk(int chunk, size_t size, void* ptr);
    };
}

#endif //TOKEN_ALLOCATOR_H