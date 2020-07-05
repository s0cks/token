#include "ms_scavenger.h"
#include "allocator.h"

namespace Token{
    class MarkSweepMemoryScavenger : public ObjectPointerVisitor{
    private:
        MarkSweepScavenger* parent_;
    public:
        MarkSweepMemoryScavenger(MarkSweepScavenger* parent):
            ObjectPointerVisitor(),
            parent_(parent){}
        ~MarkSweepMemoryScavenger(){}

        MarkSweepScavenger* GetParent() const{
            return parent_;
        }

        bool Visit(RawObject* obj){
            return GetParent()->DeleteObject(obj);
        }
    };

    bool MarkSweepScavenger::ScavengeMemory(){
#ifdef TOKEN_DEBUG
        LOG(INFO) << "performing garbage collection...";
        uintptr_t allocated_size = Allocator::allocated_size_;
#endif//TOKEN_DEBUG

        if(!DarkenRoots()) CrashReport::GenerateAndExit("Couldn't darken roots");
        if(!MarkObjects()) CrashReport::GenerateAndExit("Couldn't mark references");

        uintptr_t size = allocated_size;
        std::unordered_map<uintptr_t, RawObject*> allocated = Allocator::allocated_;
        auto current = allocated.cbegin();
        while(current != allocated.cend()){
            uintptr_t address = current->first;
            RawObject* obj = current->second;
            if(obj->GetColor() == Color::kWhite){
#ifdef TOKEN_DEBUG
                LOG(INFO) << "removed object: " << (*obj);
#endif//TOKEN_DEBUG
                size -= obj->GetObjectSize();
                delete obj;
                current = allocated.erase(current);
                continue;
            }
            current++;
        }

        Allocator::allocated_ = allocated;
        Allocator::allocated_size_ = size;
#ifdef TOKEN_DEBUG
        uintptr_t total_mem_scavenged = (allocated_size - size);
        LOG(INFO) << "garbage collection complete";
        LOG(INFO) << "collected: " << total_mem_scavenged << "/" << Allocator::GetTotalSize() << " bytes";
        LOG(INFO) << "allocated: " << size << "/" << Allocator::GetTotalSize() << " bytes";
#endif//TOKEN_DEBUG
        return true;
    }
}