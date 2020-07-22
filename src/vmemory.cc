#include <sys/mman.h>
#include "vmemory.h"

namespace Token{
    MemoryRegion::MemoryRegion(uintptr_t size):
        start_(nullptr),
        size_(0){
        if(size > 0) {
            size_ = RoundUpPowTwo(size);
            LOG(INFO) << "creating memory region of size: " << size_;
            if(!(start_ = malloc(size_))){
                LOG(WARNING) << "couldn't create memory region of size: " << size_;
                return;
            }
        }
        Clear();
    }

    MemoryRegion::~MemoryRegion(){
        if(start_!= nullptr) if(start_)free(start_);
    }

    void MemoryRegion::Clear(){
        if(!start_)return;
        memset(start_, 0, size_);
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