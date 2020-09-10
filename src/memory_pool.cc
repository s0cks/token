#include "memory_pool.h"
#include "object.h"
#include "allocator.h"

namespace Token{
    MemoryPoolCache::CacheNode::CacheNode(Object* value):
        previous_(nullptr),
        next_(nullptr),
        key_(value->GetHash()),
        value_(value){}

    bool MemoryPoolCache::PutItem(Object* value){
        uint256_t hash = value->GetHash();
        if(!ContainsItem(hash)){
            if(GetSize() > GetMaxSize()){
                if(!EvictLeastUsedItem())
                    return false;
            }
        } else{
            if(!RemoveItem(hash))
                return false;
        }

        items_.push_front(hash);
        return PutNode(hash, value);
    }

    bool MemoryPool::Accept(ObjectPointerVisitor* vis){
        uword current = GetStartAddress();
        while(current < current_){
            Object* obj = (Object*)current;
            if(!vis->Visit(obj)) return false;
            current += obj->GetSize();
        }
        return true;
    }

    bool MemoryPoolCache::Accept(ObjectPointerVisitor* vis){
        for(size_t bucket = 0; bucket < size_; bucket++){
            CacheNode* node = nodes_[bucket];
            while(node != nullptr){
                if(!vis->Visit(node->GetValue()))
                    return false;
                node = node->GetNext();
            }
        }
        return true;
    }
}