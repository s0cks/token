#ifndef TOKEN_VMEMORY_H
#define TOKEN_VMEMORY_H

#include "common.h"

namespace Token{
    class MemoryRegion{
    public:
        enum ProtectionMode{
            kNoAccess,
            kReadOnly,
            kReadWrite,
            kReadExecute,
            kReadWriteExecute
        };
    private:
        void* start_;
        uintptr_t size_;

        friend class Heap;
    public:
        MemoryRegion():
            size_(0),
            start_(nullptr){}
        MemoryRegion(uintptr_t size);
        MemoryRegion(void* ptr, uintptr_t size):
            MemoryRegion(size){
            CopyFrom(ptr, size);
        }
        MemoryRegion(const MemoryRegion& region):
            MemoryRegion(region.size_){
            CopyFrom(region);
        }
        ~MemoryRegion();

        void* GetPointer() const{
            return start_;
        }

        uintptr_t GetSize() const{
            return size_;
        }

        uintptr_t GetStart() const{
            return reinterpret_cast<uintptr_t>(start_);
        }

        uintptr_t GetEnd() const{
            return GetStart() + size_;
        }

        void CopyFrom(const MemoryRegion& other) const{
            memcpy(start_, other.start_, other.size_);
        }

        void CopyFrom(void* ptr, uintptr_t size) const{
            memcpy(start_, ptr, size);
        }

        bool Protect(ProtectionMode mode);
    };
}

#endif //TOKEN_VMEMORY_H