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
            start_(nullptr),
            size_(0){}
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

        size_t GetSize() const{
            return size_;
        }

        uword GetStartAddress() const{
            return (uword)start_;
        }

        uword GetEndAddress() const{
            return GetStartAddress() + size_;
        }

        void CopyFrom(const MemoryRegion& other) const{
            memcpy(start_, other.start_, other.size_);
        }

        void CopyFrom(void* ptr, size_t size) const{
            memcpy(start_, ptr, size);
        }

        bool Protect(ProtectionMode mode);
        void Clear();
    };
}

#endif //TOKEN_VMEMORY_H