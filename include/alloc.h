#ifndef TOKEN_ALLOC_H
#define TOKEN_ALLOC_H

#include <cstdlib>
#include <cstdint>
#include <ostream>

#define GC_MAXREFS 65536
#define GC_MINCHUNK_SIZE 2048
#define GC_MAJCHUNK_SIZE (GC_MINCHUNK_SIZE * 2)
#define GC_MINHEAP_SIZE ((GC_MINCHUNK_SIZE * 4) * 64)
#define GC_MAJHEAP_SIZE ((GC_MAJCHUNK_SIZE * 4) * 64)
#define GC_MINCHUNKS (GC_MINHEAP_SIZE / GC_MINCHUNK_SIZE)

namespace Token{
    class Allocator{
    private:
        enum class AllocColor{
            kFree = 0,
            kWhite = 1,
            kBlack,
            kGray
        };

        unsigned char minor_[GC_MINHEAP_SIZE];
        unsigned char major_[GC_MAJHEAP_SIZE];
        int free_chunk_;
        int refs_count_;
        void* backpatch_[GC_MINCHUNKS];
        void** references_[GC_MAXREFS][2];

        inline int
        GetFreeChunk() const{
            return free_chunk_;
        }

        inline void
        SetReferenceBlock(int idx, void** start, void** end){
            std::cout << "Setting reference " << idx << " " << (*start) << "-" << (*end) << std::endl;
            references_[idx][0] = start;
            references_[idx][1] = end;
        }

        inline void
        AddReferenceBlock(void** begin, void** end){
            SetReferenceBlock(refs_count_, begin, end);
            refs_count_++;
        }

        Allocator();
        void MarkChunk(void* chunk);
        void MarkMinor();
        void CopyMinorHeap();
        void BackpatchReferences();
        void BackpatchChunk(void* chunk);
        void* MinorAlloc(size_t size);
        void* MajorAlloc(size_t size);

        std::string GetChunkColor(void* chunk);
    public:
        static Allocator* GetInstance();
        static void* Alloc(size_t size);
        static void AddReference(void** begin, void** end);
        static void AddReference(void** ptr);
        bool CollectMinor();
        bool CollectMajor();

        ~Allocator();

        void PrintMinor(std::ostream& ostream);
        void PrintMajor(std::ostream& ostream);
        void PrintReferences(std::ostream& ostream);
    };
}

#endif //TOKEN_ALLOC_H