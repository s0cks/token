#ifndef TOKEN_ALLOCATOR_H
#define TOKEN_ALLOCATOR_H

#include <cstdlib>
#include <cstdint>

#define GC_MAX_REFS 65536
#define GC_MINCHUNK_SIZE 256
#define GC_MINHEAP_SIZE (1024 * 64)
#define GC_MAJHEAP_SIZE (1024 * 1024 * 32)
#define GC_MINCHUNKS (GC_MINHEAP_SIZE / GC_MINCHUNK_SIZE)

namespace Token{
    class HeapVisitor;

    class Allocator{
    public:
        typedef uint8_t Byte;

        enum class Color{
            kFree = 0,
            kWhite,
            kBlack,
            kGray,
            kMarked = kWhite,
        };
    private:
        Byte minor_[GC_MINHEAP_SIZE];
        Byte major_[GC_MAJHEAP_SIZE];
        int free_chunk_;
        int refs_count_;
        void* backpatch_[GC_MINCHUNKS];
        void** references_[GC_MAX_REFS][2];

        inline int
        GetFreeChunk() const{
            return free_chunk_;
        }

        inline int
        GetNumberOfReferences() const{
            return refs_count_;
        }

        void SetReference(int idx, void* start, void* end);
        void AddReference(void** begin, void** end);
        void MarkChunk(Byte* ch);
        void BackpatchReferences();
        void BackpatchChunk(Byte* ch);
        void CopyMinorHeap();
        void MarkMinor();
        void VisitMinor(HeapVisitor* vis);
        void VisitMajor(HeapVisitor* vis);
        void* MinorAlloc(size_t size);
        void* MajorAlloc(size_t size);
        static Allocator* GetInstance();

        Allocator();

        friend class HeapPrinter;
    public:
        ~Allocator();

        static void AddReference(void* ref);
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
        virtual bool VisitChunk(int chunk, size_t size, void* ptr) = 0;
    };

    class HeapPrinter : public HeapVisitor{
    public:
        HeapPrinter(){}
        ~HeapPrinter(){}

        bool VisitChunk(int chunk, size_t size, void* ptr);
    };
}

#endif //TOKEN_ALLOCATOR_H