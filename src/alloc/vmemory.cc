#include <sys/mman.h>
#include "vmemory.h"

namespace Token{
    MemoryRegion::MemoryRegion(uintptr_t size):
        start_(nullptr),
        size_(0){
        if(size > 0) {
            size_ = RoundUpPowTwo(size);
#if defined(TOKEN_VMEMORY_USE_MEMORYMAP)
#warning "compiling token with memory mapped regions!"
            if ((start_ = mmap(nullptr, size_, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0)) == MAP_FAILED) {
                LOG(WARNING) << "couldn't map memory region of size: " << size_;
                return;
            }
#else
            LOG(INFO) << "creating memory region of size: " << size_;
            if(!(start_ = malloc(size_))){
                LOG(WARNING) << "couldnt' create memory region of size: " << size_;
                return;
            }
#endif
            memset(start_, 0, size);
        }
    }

    MemoryRegion::~MemoryRegion(){
        if(start_!= nullptr){
#if defined(TOKEN_VMEMORY_USE_MEMORYMAP)
            if(munmap(start_, size_) != 0){
                LOG(WARNING) << "failed to munmap memory region!";
                return;
            }
#else
            if(start_)free(start_);
#endif
        }
    }

    bool MemoryRegion::Protect(ProtectionMode mode){
        int prot = 0;
        switch(mode){
            case kNoAccess: prot = PROT_NONE; break;
            case kReadOnly: prot = PROT_READ; break;
            case kReadWrite: prot = PROT_READ|PROT_WRITE; break;
            case kReadExecute: prot = PROT_READ|PROT_EXEC; break;
            case kReadWriteExecute: prot = PROT_READ|PROT_WRITE|PROT_EXEC; break;
        }
        return (mprotect(GetPointer(), GetSize(), prot) == 0);
    }
}