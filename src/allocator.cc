#include <cstring>
#include <cstdio>
#include <glog/logging.h>
#include "common.h"
#include "allocator.h"
#include "printer.h"

#define WITH_HEADER(size) ((size)+sizeof(int))
#define WITHOUT_HEADER(size) ((size)-sizeof(int))
#define ALIGN(ptr) (((ptr)+3)&~3)
#define CHUNK_AT(idx) (&minor_[(idx)*kMinChunkSize])
#define BITS(ch) (WITH_HEADER(ch))
#define FLAGS(ch) (*((uint32_t*)(ch)))
#define MARK_CHUNK(ch, c) ((FLAGS(ch)) = CHUNK_SIZE(ch)|static_cast<uint32_t>((c)))
#define CHUNK_SIZE(ch) (FLAGS((ch))&(~3))
#define CHUNK_FLAGS(ch) (static_cast<Allocator::Color>((FLAGS((ch))&(3))))
#define MEM_TAG(ptr) (!(((uintptr_t)(ptr))&1))
#define POINTS_MINOR(ptr) (((ptr))>=&minor_[0] && ((ptr))<=&minor_[minor_size_])
#define POINTS_MAJOR(ptr) (((Byte*)(ptr))>=&major_[0] && ((Byte*)(ptr))<=&major_[major_size_])
#define REF_PTR(ptr) (MEM_TAG((ptr))&&POINTS_MINOR((ptr)))
#define MINOR_CHUNK(ptr) (((((Byte*)(ptr)-&minor_[0])/kMinChunkSize)*kMinChunkSize)+&minor_[0])
#define PTR_INDEX(ofs) ((ofs)/sizeof(void*))
#define BITS(ch) (WITH_HEADER(ch))
#define BITS_AT(ch, idx) ((((void**)(BITS((ch))+(idx)*sizeof(void*)))))
#define MARKED(ch) (FLAGS(ch)&1)
#define CHUNK_OFFSET(ch) ((((ch))-&minor_[0])/kMinChunkSize)
#define REF_PTR_MJ(ptr) (MEM_TAG((ptr)) && POINTS_MAJOR((ptr)))

#define FOR_EACH_MINCH(ch) for(i=0; i<free_chunk_; i++){ \
    Byte* ch = CHUNK_AT(i); do{}while(0)
#define END_FOR_EACH_MINCH() }
#define FOR_EACH_REFERENCE(ref, counter) \
    int counter; \
    for(counter=0; counter<refs_count_; counter++){ \
        void* ref = references_[counter];
#define END_FOR_EACH_REFERENCE() }

namespace Token{
    Allocator::Allocator():
        free_chunk_(0),
        refs_count_(0),
        minor_(nullptr),
        major_(nullptr),
        backpatch_(nullptr),
        references_(){
    }

    Allocator::~Allocator(){}

    Allocator* Allocator::GetInstance(){
        static Allocator instance;
        return &instance;
    }

    void* Allocator::MajorAlloc(size_t size){
        if(size == 0) return NULL;
        size = WITH_HEADER(ALIGN(size));
        Byte* curr;
        for(curr = &major_[0];
            (CHUNK_FLAGS(curr) != Color::kFree || size > CHUNK_SIZE(curr)) &&
            curr < &major_[major_size_];
            curr = curr + CHUNK_SIZE(curr));
        if(curr >= &major_[major_size_]){
            CollectMajor();
            return MajorAlloc(size);
        }
        Byte* fchunk = curr;
        unsigned int prev_size = CHUNK_SIZE(fchunk);
        FLAGS(fchunk) = size;
        MARK_CHUNK(fchunk, Color::kWhite);
        Byte* next_chunk = (curr + size);
        FLAGS(next_chunk) = prev_size - size;
        return (void*)WITH_HEADER(fchunk);
    }

    void* Allocator::MinorAlloc(size_t size){
        if(size == 0) return NULL;
        size = (size_t)WITH_HEADER(ALIGN(size));
        if(size > kMinChunkSize) return NULL;
        if(GetFreeChunk() >= (minor_size_ / kMinChunkSize)){
            CollectMinor();
            return MinorAlloc(WITHOUT_HEADER(size));
        }
        FLAGS(CHUNK_AT(free_chunk_)) = size;
        return BITS(CHUNK_AT(free_chunk_++));
    }

    void* Allocator::Allocate(size_t size){
        Allocator* instance = GetInstance();
        void* chunk = instance->MinorAlloc(size);
        if(!chunk) return instance->MajorAlloc(size);
        return chunk;
    }

    void Allocator::MarkChunk(Token::Allocator::Byte* ch){
        MARK_CHUNK(ch, Color::kMarked);
        int i;
        for(i = 0; i < PTR_INDEX(CHUNK_SIZE(ch)); i++){
            if(REF_PTR(*BITS_AT(ch, i))){
                Byte* ptr = reinterpret_cast<Byte*>(*BITS_AT(ch, i));
                Byte* ref = MINOR_CHUNK(ptr);
                int idx = (ch - &minor_[0]);
                if(!MARKED(ref)){
                    MarkChunk(ref);
                }
            }
        }
    }

    void Allocator::BackpatchChunk(Token::Allocator::Byte* ch){
        int i;
        for(i = 0; i < PTR_INDEX(CHUNK_SIZE(ch)); i++){
            if(REF_PTR(*BITS_AT(ch, i))){
                Byte* ptr = reinterpret_cast<Byte*>(*BITS_AT(ch, i));
                Byte* ref = MINOR_CHUNK(ptr);
                if(MARKED(ref)){
                    Byte* nnptr = (Byte*)backpatch_[CHUNK_OFFSET(ref)];
                    int delta = nnptr - ref;
                    *BITS_AT(ch, i) += delta;
                }
            }
        }
    }

    void Allocator::BackpatchReferences(){
        FOR_EACH_REFERENCE(ptr, i)
        {
            if(POINTS_MINOR(ptr)){
                Byte* ch = MINOR_CHUNK(ptr);
                if(MARKED(ch)){
                    Byte* nptr = (Byte*)backpatch_[CHUNK_OFFSET(ch)];
                    if(nptr){
                        int delta = nptr - ch;
                        ptr += delta;
                    }
                }
            }
        }
        END_FOR_EACH_REFERENCE();
    }

    uint64_t Allocator::GetMinorHeapSize(){
        return GetInstance()->minor_size_;
    }

    uint64_t Allocator::GetMajorHeapSize(){
        return GetInstance()->major_size_;
    }

    bool Allocator::Initialize(uint64_t minheap_size, uint64_t maxheap_size){
        GetInstance()->minor_size_ = RoundUpPowTwo(minheap_size);
        GetInstance()->major_size_ = RoundUpPowTwo(maxheap_size);

        LOG(WARNING) << "Minor Heap Size: " << GetMinorHeapSize();
        LOG(WARNING) << "Major Heap Size: " << GetMajorHeapSize();

        Allocator* alloc = Allocator::GetInstance();
        alloc->minor_ = (Byte*)malloc(sizeof(Byte) * GetMinorHeapSize());
        alloc->major_ = (Byte*)malloc(sizeof(Byte) * GetMajorHeapSize());
        alloc->backpatch_ = (void**)malloc(sizeof(void*) * (GetInstance()->minor_size_ / kMinChunkSize));

        memset(alloc->backpatch_, 0, sizeof(backpatch_));
        memset(alloc->minor_, 0, sizeof(minor_));
        memset(alloc->major_, 0, sizeof(major_));
        FLAGS(&alloc->major_[0]) = maxheap_size;
        return true;
    }

    void Allocator::CopyMinorHeap(){
        int i;
        FOR_EACH_MINCH(ch);
            {
                if(MARKED(ch)){
                    LOG(WARNING) << "copying reference: " << ch;
                    void* ptr = WITHOUT_HEADER(MajorAlloc(WITHOUT_HEADER(CHUNK_SIZE(ch))));
                    backpatch_[i] = ptr;
                }
            }
        END_FOR_EACH_MINCH();
        BackpatchReferences();
        FOR_EACH_MINCH(ch);
            {
                BackpatchChunk(ch);
            }
        END_FOR_EACH_MINCH();
        FOR_EACH_MINCH(ch);
            {
                if(MARKED(ch)){
                    Byte* nptr = (Byte*)backpatch_[i];
                    memcpy(nptr, ch, CHUNK_SIZE(ch));
                }
            }
        END_FOR_EACH_MINCH();
        free_chunk_ = 0;
    }

    void Allocator::MarkMinor(){
        memset(backpatch_, 0, sizeof(backpatch_));
        FOR_EACH_REFERENCE(ref, i);
        {
            LOG(INFO) << "visiting reference: " << i << " (" << (void*)ref << ")";
            if(POINTS_MINOR(ref)){
                Byte* ch = MINOR_CHUNK(ref);
                LOG(INFO) << "marking chunk: " << (void*)ch;
                MarkChunk(ch);
            }
        }
        END_FOR_EACH_REFERENCE();
    }

    void Allocator::CollectMinor(){
        LOG(WARNING) << "collecting minor...";
        GetInstance()->MarkMinor();
        GetInstance()->CopyMinorHeap();
        LOG(WARNING) << "done";
    }

    Allocator::Byte* Allocator::FindMajorChunk(Token::Allocator::Byte* ptr){
        Byte* curr;
        for(curr = &major_[0];
            !(ptr >= curr && (ptr < (curr + CHUNK_SIZE(curr))))
            && curr < &major_[major_size_];
            curr += CHUNK_SIZE(curr)){
            if(curr < &major_[major_size_]) return curr;
        }
        return NULL;
    }

    void Allocator::DarkenChunk(Token::Allocator::Byte* ch){
        Byte* curr = FindMajorChunk(ch);
        if(curr != 0 && CHUNK_FLAGS(curr) == Color::kWhite){
            MARK_CHUNK(curr, Color::kGray);
        }
    }

    void Allocator::MarkMajorChunk(Token::Allocator::Byte* ch){
        if(CHUNK_FLAGS(ch) != Color::kBlack){
            MARK_CHUNK(ch, static_cast<Allocator::Color>(static_cast<int>(CHUNK_FLAGS(ch)) + 1));
            int i;
            for(i = 0; i < PTR_INDEX(CHUNK_SIZE(ch)); i++){
                if(REF_PTR_MJ(*BITS_AT(ch, i))){
                    Byte* ptr = reinterpret_cast<Byte*>(*BITS_AT(ch, i));
                    Byte* ref = FindMajorChunk(ptr);
                    MarkMajorChunk(ref);
                }
            }
        }
    }

    void Allocator::DarkenMajor(){
        Byte* curr;
        for(curr = &major_[0];
            curr < &major_[major_size_];
            curr += CHUNK_SIZE(curr)){
            switch(CHUNK_FLAGS(curr)){
                case Color::kGray:
                    MarkMajorChunk(curr);
                    break;
                default: break;
            }
        }
    }

    void Allocator::DarkenRoots(){
        FOR_EACH_REFERENCE(ptr, i)
            {
                DarkenChunk((Byte*)ptr);
            }
        END_FOR_EACH_REFERENCE();
    }

    void Allocator::CollectMajor(){
        GetInstance()->DarkenRoots();
        GetInstance()->DarkenMajor();

        Byte* curr;
        for(curr = &GetInstance()->major_[0];
            curr < &GetInstance()->major_[GetInstance()->major_size_];
            curr += CHUNK_SIZE(curr)){
            switch(CHUNK_FLAGS(curr)){
                case Color::kWhite:
                    MARK_CHUNK(curr, Color::kFree);
                    break;
                default: break;
            }
        }
    }

    void Allocator::VisitMinor(Token::HeapVisitor* vis){
        vis->VisitStart();
        int i;
        FOR_EACH_MINCH(ch);
            if(CHUNK_SIZE(ch) == 0) return;
            vis->VisitChunk(CHUNK_OFFSET(ch), CHUNK_SIZE(ch), ch);
        END_FOR_EACH_MINCH();
        vis->VisitEnd();
    }

    void Allocator::VisitMajor(Token::HeapVisitor* vis){
        vis->VisitStart();
        int i = 0;
        Byte* curr;
        for(curr = &major_[0];
            curr < &major_[major_size_];
            curr = curr + CHUNK_SIZE(curr)){
            if(CHUNK_SIZE(curr) == 0 || CHUNK_FLAGS(curr) == Color::kFree) break;
            vis->VisitChunk(i++, CHUNK_SIZE(curr), curr);
        }
        vis->VisitEnd();
    }

    void Allocator::SetReference(int idx, void* ref){
        references_[idx] = ref;
    }

    void Allocator::AddReference(void* ref){
        int i;
        for(i = 0; i < GetInstance()->refs_count_; i++){
            if(GetInstance()->references_[i] == ref) return;
        }
        GetInstance()->SetReference(GetInstance()->refs_count_++, ref);
    }

    void Allocator::RemoveReference(void *ref){
        //TODO: Implement
    }
}