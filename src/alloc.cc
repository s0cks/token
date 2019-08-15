#include <cstring>
#include <iostream>
#include "alloc.h"

#define WITH_HEADER(size) ((size)+sizeof(int))
#define WITHOUT_HEADER(size) ((size)-sizeof(int))
#define FLAGS(chunk) (*(reinterpret_cast<unsigned int*>(chunk)))
#define CHUNK_SIZE(chunk) (FLAGS((chunk))&(~3))
#define MARK(chunk, c) ((FLAGS(chunk)) = CHUNK_SIZE(chunk)|static_cast<unsigned int>(c))
#define PTR(ofs) ((ofs) / sizeof(void*))
#define BITS(chunk) (reinterpret_cast<void*>(WITH_HEADER(chunk)))
#define BITS_AT(chunk, idx) (reinterpret_cast<void**>(BITS((chunk))+(idx)*sizeof(void*)))
#define MEM(ptr) (!(((reinterpret_cast<unsigned long>(ptr))&1)))
#define POINTS_MINOR(ptr) (((ptr)>=&minor_[0]&&((ptr) < &minor_[GC_MINHEAP_SIZE])))
#define POINTS_MAJOR(ptr) (((Byte*)(ptr)) >= &major_[0] && ((Byte*)(ptr)) < &major_[GC_MAJHEAP_SIZE])
#define MARKED(chunk) (FLAGS(chunk)&static_cast<unsigned int>(AllocColor::kWhite))
#define REF_PTR_MIN(ptr) (MARKED(ptr)&&POINTS_MINOR(ptr))
#define REF_PTR_MAJ(ptr) (MEM((ptr)) && POINTS_MAJOR((ptr)))
#define MINOR_CHUNK(ptr) (((((unsigned char*)(ptr)-&minor_[0])/GC_MINCHUNK_SIZE)*GC_MINCHUNK_SIZE)+&minor_[0])
#define CHUNK_FLAGS(chunk) (static_cast<AllocColor>(FLAGS((chunk)) & (3)))
#define ALIGN(ptr) (((ptr)+3) & ~3)
#define CHUNK_AT(idx) (&minor_[(idx)*GC_MINCHUNK_SIZE])
#define MAJOR_CHUNK_AT(idx) (&major_[(idx) * GC_MAJCHUNK_SIZE])
#define CHUNK_OFFSET(chunk) ((((unsigned char*)(chunk))-&minor_[0]) / GC_MINCHUNK_SIZE)


namespace Token{
    Allocator* Allocator::GetInstance(){
        static Allocator instance;
        return &instance;
    }

    Allocator::Allocator():
        major_(),
        minor_(),
        free_chunk_(0),
        refs_count_(0),
        backpatch_(),
        references_(){
        memset(references_, 0, sizeof(references_));
        memset(minor_, 0, sizeof(minor_));
        memset(major_, 0, sizeof(major_));
        FLAGS(&major_[0]) = GC_MAJHEAP_SIZE;
    }

    Allocator::~Allocator(){
        // TODO: Implement?
    }

    void* Allocator::MinorAlloc(size_t size){
        if(size == 0) return nullptr;
        size = WITH_HEADER(ALIGN(size));
        if(size > GC_MINCHUNK_SIZE) return MajorAlloc(size);
        if(GetFreeChunk() > GC_MINCHUNKS){
            CollectMinor();
            return MinorAlloc(WITHOUT_HEADER(size));
        }
        FLAGS(CHUNK_AT(free_chunk_)) = ((unsigned int)size);
        return BITS(CHUNK_AT(free_chunk_++));
    }

    void* Allocator::MajorAlloc(size_t size){
        if(size == 0) return nullptr;
        size = WITH_HEADER(ALIGN(size));
        void* curr;
        for(curr = &major_[0];
            (CHUNK_FLAGS(curr) != AllocColor::kFree || size > CHUNK_SIZE(curr)) &&
            curr < &major_[GC_MAJHEAP_SIZE];
            curr += CHUNK_SIZE(curr));
        if(curr >= &major_[GC_MAJHEAP_SIZE]) return nullptr;
        void* free_chunk = curr;
        unsigned int prev_size = CHUNK_SIZE(free_chunk);
        FLAGS(free_chunk) = size;
        MARK(free_chunk, AllocColor::kWhite);
        void* next_chunk = (curr + size);
        FLAGS(next_chunk) = ((unsigned int)(prev_size - size));
        return ((void*) WITH_HEADER(free_chunk));
    }

    void Allocator::MarkChunk(void* chunk){
        std::cout << "Marking: " << chunk << std::endl;
        MARK(chunk, 1);
        for(int i = 0; i < PTR(CHUNK_SIZE(chunk)); i++){
            if(REF_PTR_MIN(*BITS_AT(chunk, i))){
                void* ptr = *BITS_AT(chunk, i);
                void* ref = MINOR_CHUNK(ptr);
                std::cout << "reference: " << ptr << " <-> " << ref << std::endl;
                if(!MARKED(ref)){
                    MarkChunk(ref);
                }
            }
        }
    }

    void Allocator::MarkMinor(){
        memset(backpatch_, 0, sizeof(backpatch_));
        int counter;
        for(counter = 0; counter < refs_count_; counter++){
            void** ref;
            for(ref = references_[counter][0]; ref < references_[counter][1]; ref++){
                if(ref != NULL && REF_PTR_MIN(*ref)){
                    std::cout << "Marking minor chunk: " << (*ref) << std::endl;
                    MarkChunk(MINOR_CHUNK(*ref));
                }
            }
        }
    }

    void Allocator::BackpatchChunk(void* chunk){
        int i;
        for(i = 0; i < PTR(CHUNK_SIZE(chunk)); i++){
            if(REF_PTR_MIN(*BITS_AT(chunk, i))){
                void* ptr = *BITS_AT(chunk, i);
                void* ref = MINOR_CHUNK(ptr);
                if(MARKED(ref)){
                    void* nptr = backpatch_[CHUNK_OFFSET(ref)];
                    *BITS_AT(chunk, i) += (((char*)nptr) - ((char*)ref));
                }
            }
        }
    }

    void Allocator::BackpatchReferences(){
        int counter;
        for(counter = 0; counter < refs_count_; counter++){
            void** ref;
            for(ref = references_[counter][0];
                ref < references_[counter][1];
                ref++){
                if(ref != 0){
                    if(REF_PTR_MIN(*ref)){
                        std::cout << "Minor chunk ptr" << std::endl;
                        void* chunk = MINOR_CHUNK(*ref);
                        if(MARKED(chunk)){
                            void* nptr = backpatch_[CHUNK_OFFSET(chunk)];
                            std::cout << "Backpatching Reference #" << counter << " := " << nptr << std::endl;
                            if(nptr != 0){
                                *ref += (((char*)nptr) - ((char*)chunk));
                            }
                        }
                    }
                }
            }
        }
    }

    void Allocator::CopyMinorHeap(){
        int i;
        for(i = 0; i < GetFreeChunk(); i++){
            void* chunk = CHUNK_AT(i);
            if(CHUNK_FLAGS(chunk) == AllocColor::kWhite){
                std::cout << "Backpatching chunk " << i << std::endl;
                void* ptr = WITHOUT_HEADER(MajorAlloc(WITHOUT_HEADER(CHUNK_SIZE(chunk))));
                std::cout << "Backpatch[" << i << "] := " << ptr << std::endl;
                backpatch_[i] = ptr;
            }
        }
        BackpatchReferences();
        for(i = 0; i < GetFreeChunk(); i++){
            BackpatchChunk(CHUNK_AT(i));
        }
        for(i = 0; i < GetFreeChunk(); i++){
            void* chunk = CHUNK_AT(i);
            if(CHUNK_FLAGS(chunk) == AllocColor::kWhite){
                std::cout << "Copying backpatched chunk #" << i << " " << CHUNK_SIZE(chunk) << "bytes" << std::endl;
                void* nchunk = backpatch_[i];
                std::cout << chunk << "  ==>  " << nchunk << std::endl;
                memcpy(nchunk, chunk, CHUNK_SIZE(chunk));
            }
        }
    }

    bool Allocator::CollectMinor(){
        MarkMinor();
        CopyMinorHeap();
        free_chunk_ = 0;
        return true;
    }

    void Allocator::PrintMinor(std::ostream &ostream){
        ostream << "*** Minor Heap Chunks ***" << std::endl;
        ostream << "  + Number of Chunks: " << free_chunk_ << std::endl;
        ostream << "  + Chunks: " << std::endl;
        int i;
        for(i = 0; i < free_chunk_; i++){
            void* chunk = CHUNK_AT(i);
            std::string flags = GetChunkColor(chunk);
            ostream << "\t- " << i << ". Chunk  " << (void*)chunk << ": Size=" << CHUNK_SIZE(chunk) << "; Marked=" << flags << std::endl;
        }
        ostream << "*** End of Minor Heap ****" << std::endl;
    }

    std::string Allocator::GetChunkColor(void* chunk){
        switch(CHUNK_FLAGS(chunk)){
            case AllocColor::kFree: return "free";
            case AllocColor::kWhite: return "white";
            case AllocColor::kGray: return "gray";
            case AllocColor::kBlack: return "black";
        }
    }

    void Allocator::PrintMajor(std::ostream &ostream){
        ostream << "*** Major Heap Chunks ***" << std::endl;
        ostream << "  + Chunks: " << std::endl;

        int i = 0;
        void* current;
        for(current = &major_[0];
            current < &major_[GC_MAJHEAP_SIZE];
            current = current + CHUNK_SIZE(current)){
            void* chunk = MAJOR_CHUNK_AT(i++);
            if(CHUNK_SIZE(chunk) == 0 && CHUNK_FLAGS(chunk) == AllocColor::kFree) break;
            std::string flags = GetChunkColor(chunk);
            ostream << "\t- " << i << ". Chunk " << (void*)chunk << ": Size=" << CHUNK_SIZE(chunk) << "; Marked=" << flags << std::endl;
        }
        ostream << "*** End of Major Heap ****" << std::endl;
    }

    void* Allocator::Alloc(size_t size){
        Allocator* instance = GetInstance();
        void* ptr = instance->MinorAlloc(size);
        if(!ptr) return instance->MajorAlloc(size);
        return ptr;
    }

#define CHECK_CHUNK(begin, end)({ \
  if(begin == end){ \
    return; \
  } \
  if(begin > end){ \
    void** tmp = begin; \
    begin = end; \
    end = tmp; \
  } \
})

    void Allocator::AddReference(void** begin, void** end){
        CHECK_CHUNK(begin, end);
        Allocator* instance = GetInstance();
        for(int i = 0; i < instance->refs_count_; i++){
            if(begin >= instance->references_[i][0] && end <= instance->references_[i][1]) return;
        }
        for(int i = 0; i < instance->refs_count_; i++){
            if(instance->references_[i][0] == 0){
                instance->SetReferenceBlock(i, begin, end);
                return;
            }
        }
        instance->AddReferenceBlock(begin, end);
    }

    void Allocator::AddReference(void** ptr){
        AddReference(ptr, ptr + sizeof(void*));
    }
}