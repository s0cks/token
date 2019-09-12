#include <cstring>
#include <cstdio>
#include <glog/logging.h>
#include "allocator.h"

#define WITH_HEADER(size) ((size)+sizeof(int))
#define WITHOUT_HEADER(size) ((size)-sizeof(int))
#define ALIGN(ptr) (((ptr)+3)&~3)
#define CHUNK_AT(idx) (&minor_[(idx)*GC_MINCHUNK_SIZE])
#define BITS(ch) (WITH_HEADER(ch))
#define FLAGS(ch) (*((uint32_t*)(ch)))
#define MARK_CHUNK(ch, c) ((FLAGS(ch)) = CHUNK_SIZE(ch)|c)
#define CHUNK_SIZE(ch) (FLAGS((ch))&(~3))
#define CHUNK_FLAGS(ch) (static_cast<Allocator::Color>((FLAGS((ch))&(3))))
#define MEM_TAG(ptr) (!(((uint64_t)(ptr))&1))
#define POINTS_MINOR(ptr) (((Byte*)(ptr))>=&minor_[0] && ((Byte*)(ptr))<&minor_[GC_MINHEAP_SIZE])
#define REF_PTR(ptr) (MEM_TAG((ptr))&&POINTS_MINOR((ptr)))
#define MINOR_CHUNK(ptr) (((((Byte*)(ptr)-&minor_[0])/GC_MINCHUNK_SIZE)*GC_MINCHUNK_SIZE)+&minor_[0])
#define PTR_INDEX(ofs) ((ofs)/sizeof(void*))
#define BITS(ch) (WITH_HEADER(ch))
#define BITS_AT(ch, idx) ((((void**)(BITS((ch))+(idx)*sizeof(void*)))))
#define MARKED(ch) (FLAGS(ch)&1)
#define CHUNK_OFFSET(ch) ((((ch))-&minor_[0])/GC_MINCHUNK_SIZE)

#define FOR_EACH_MINCH(ch) for(i=0; i<free_chunk_; i++){ \
    Byte* ch = CHUNK_AT(i); do{}while(0)
#define END_FOR_EACH_MINCH() }
#define FOR_EACH_REFERENCE(ref, counter) \
    int counter; \
    for(counter=0; counter<refs_count_; counter++){ \
        void** ref = references_[counter][0];
#define END_FOR_EACH_REFERENCE() }

namespace Token{
    Allocator::Allocator():
        free_chunk_(0),
        refs_count_(0),
        minor_(),
        major_(),
        backpatch_(),
        references_(){
        memset(backpatch_, 0, sizeof(backpatch_));
        memset(minor_, 0, sizeof(minor_));
        memset(major_, 0, sizeof(major_));
        FLAGS(&major_[0]) = GC_MAJHEAP_SIZE;
    }

    Allocator::~Allocator(){

    }

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
            curr < &major_[GC_MAJHEAP_SIZE];
            curr = curr + CHUNK_SIZE(curr));
        if(curr >= &major_[GC_MAJHEAP_SIZE]) return NULL;
        Byte* fchunk = curr;
        unsigned int prev_size = CHUNK_SIZE(fchunk);
        FLAGS(fchunk) = size;
        MARK_CHUNK(fchunk, static_cast<unsigned int>(Color::kWhite));
        Byte* next_chunk = (curr + size);
        FLAGS(next_chunk) = prev_size - size;
        return (void*)WITH_HEADER(fchunk);
    }

    void* Allocator::MinorAlloc(size_t size){
        if(size == 0) return NULL;
        size = (size_t)WITH_HEADER(ALIGN(size));
        if(size > GC_MINCHUNK_SIZE) return NULL;
        if(GetFreeChunk() >= GC_MINCHUNKS){
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
        MARK_CHUNK(ch, static_cast<unsigned int>(Color::kMarked));
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
            if(POINTS_MINOR(*ptr)){
                Byte* ch = MINOR_CHUNK(*ptr);
                if(MARKED(ch)){
                    Byte* nptr = (Byte*)backpatch_[CHUNK_OFFSET(ch)];
                    if(nptr){
                        int delta = nptr - ch;
                        *ptr += delta;
                    }
                }
            }
        }
        END_FOR_EACH_REFERENCE();
    }

    void Allocator::CopyMinorHeap(){
        int i;
        FOR_EACH_MINCH(ch);
            {
                if(MARKED(ch)){
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
            if(REF_PTR(*ref)){
                Byte* ch = MINOR_CHUNK(*ref);
                LOG(INFO) << "marking chunk: " << (void*)ch;
                MarkChunk(ch);
            }
        }
        END_FOR_EACH_REFERENCE();
    }

    void Allocator::CollectMinor(){
        GetInstance()->MarkMinor();
        GetInstance()->CopyMinorHeap();
    }

    void Allocator::CollectMajor(){

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
            curr < &major_[GC_MAJHEAP_SIZE];
            curr = curr + CHUNK_SIZE(curr)){
            if(CHUNK_SIZE(curr) == 0) return;
            vis->VisitChunk(i++, CHUNK_SIZE(curr), curr);
        }
        vis->VisitEnd();
    }

    bool HeapPrinter::VisitStart(){
        std::stringstream msg;

        int total = HeapPrinter::BANNER_SIZE;
        std::string title = space_ == kEden ?
                " Eden " :
                " Survivor ";

        total = (total - title.length()) / 2;
        for(int i = 0; i < total; i++){
            msg << "*";
        }
        msg << title;
        for(int i = 0; i < total; i++){
            msg << "*";
        }
        LOG(INFO) << msg.str();
    }

    bool HeapPrinter::VisitEnd(){
        std::stringstream msg;
        for(int i = 0; i < HeapPrinter::BANNER_SIZE; i++){
            msg << "*";
        }
        LOG(INFO) << msg.str();
    }

    bool HeapPrinter::VisitChunk(int chunk, size_t size, void* ptr){
        std::stringstream msg;
        msg << "\tChunk " << chunk << "\tSize " << size << "\tMarked: " << (MARKED(ptr) ? 'Y' : 'N');
        LOG(INFO) << msg.str();
        return true;
    }

    void Allocator::PrintMinorHeap(){
        HeapPrinter printer(HeapPrinter::kEden);
        GetInstance()->VisitMinor(&printer);
    }

    void Allocator::PrintMajorHeap(){
        HeapPrinter printer(HeapPrinter::kSurvivor);
        GetInstance()->VisitMajor(&printer);
    }

    void Allocator::SetReference(int idx, void *start, void *end){
        references_[idx][0] = (void**)start;
        references_[idx][1] = (void**)end;
    }

    void Allocator::AddReference(void **begin, void **end){
        if(begin == end) return;
        if(begin > end){
            void** tmp = begin;
            begin = end;
            end = tmp;
        }

        int i;
        for(i = 0; i < refs_count_; i++){
            if(begin >= references_[i][0] && end <= references_[i][0]) return;
        }

        for(i = 0; i < refs_count_; i++){
            if(references_[i][0] == 0){
                SetReference(i, begin, end);
                return;
            }
        }
        SetReference(refs_count_++, begin, end);
    }

    void Allocator::AddReference(void* ref){
        GetInstance()->AddReference((void**)ref, (void**)ref+sizeof(void*));
    }
}