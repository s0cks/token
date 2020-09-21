#ifndef TOKEN_MEMORY_POOL_H
#define TOKEN_MEMORY_POOL_H

#include "vmemory.h"
#include "uint256_t.h"
#include "handle.h"

namespace Token{
    class ObjectPointerVisitor;
    class MemoryPool{
        friend class Allocator;
    private:
        std::mutex mutex_;
        uword start_;
        uword current_;
        size_t size_;

        MemoryPool(uword start, size_t size):
            start_(start),
            current_(start),
            size_(size){}
    public:
        ~MemoryPool() = default;

        void* Allocate(size_t size){
            if((GetAllocatedSize() + size) > GetSize()) return nullptr;
            void* ptr = (void*)current_;
            if(!ptr) return nullptr;
            current_ += size;
            memset(ptr, 0, size);
            return ptr;
        }

        uword GetStartAddress() const{
            return start_;
        }

        uword GetEndAddress() const{
            return start_ + size_;
        }

        size_t GetSize() const{
            return size_;
        }

        size_t GetAllocatedSize(){
            std::lock_guard<std::mutex> guard(mutex_);
            return current_ - GetStartAddress();
        }

        size_t GetUnallocatedSize(){
            std::lock_guard<std::mutex> guard(mutex_);
            return GetEndAddress() - current_;
        }

        bool Contains(uword address){
            return address >= GetStartAddress()
                && address <= GetEndAddress();
        }

        bool Accept(ObjectPointerVisitor* vis);
    };

    class MemoryPoolCache{
        class CacheNode{
            friend class MemoryPoolCache;
        private:
            CacheNode* previous_;
            CacheNode* next_;
            uint256_t key_;
            Object* value_;

            CacheNode(Object* value);

            void SetNext(CacheNode* node){
                next_ = node;
            }

            void SetPrevious(CacheNode* node){
                previous_ = node;
            }

            inline static void
            WriteBarrier(Object** slot, Object* value){
                (*slot) = value;
            }

            template<typename T>
            inline static void
            WriteBarrier(T** slot, const Handle<T>& value){
                WriteBarrier(slot, (T*)value);
            }
        public:
            ~CacheNode() = default;

            CacheNode* GetNext() const{
                return next_;
            }

            bool HasNext() const{
                return next_ != nullptr;
            }

            CacheNode* GetPrevious() const{
                return previous_;
            }

            bool HasPrevious() const{
                return previous_ != nullptr;
            }

            uint256_t GetKey() const{
                return key_;
            }

            Object* GetValue() const{
                return value_;
            }
        };
    private:
        MemoryPool* pool_;
        std::deque<uint256_t> items_;
        CacheNode** nodes_;
        size_t size_;

        CacheNode* GetNode(const uint256_t& hash){
            unsigned long bucket = hash.GetHashCode() % size_;
            CacheNode* entry = nodes_[bucket];
            while(entry != nullptr){
                if(entry->GetKey() == hash) return entry;
                entry = entry->GetNext();
            }
            return nullptr;
        }

        bool RemoveNode(const uint256_t& hash){
            unsigned long bucket = hash.GetHashCode() % size_;
            CacheNode* previous = nullptr;
            CacheNode* entry = nodes_[bucket];
            while(entry != nullptr && entry->GetKey() != hash){
                previous = entry;
                entry = entry->GetNext();
            }

            if(entry == nullptr) return false;
            if(previous == nullptr){
                nodes_[bucket] = entry->GetNext();
            } else{
                previous->SetNext(entry->GetNext());
            }

            delete entry;
            return true;
        }

        bool PutNode(const uint256_t& hash, Object* value){
            unsigned long bucket = hash.GetHashCode() % size_;
            CacheNode* previous = nullptr;
            CacheNode* entry = nodes_[bucket];
            while(entry != nullptr && entry->GetKey() != hash){
                previous = entry;
                entry = entry->GetNext();
            }

            if(entry == nullptr){
                entry = new CacheNode(value);
                if(previous == nullptr){
                    nodes_[bucket] = entry;
                } else{
                    previous->SetNext(entry);
                }
                return true;
            }

            //TODO: entry->SetValue(value);
            return true;
        }

        bool EvictLeastUsedItem(){
            uint256_t last = items_.back();
            items_.pop_back();
            return RemoveNode(last);
        }
    public:
        MemoryPoolCache(MemoryPool* pool, size_t size):
            pool_(pool),
            items_(),
            nodes_(nullptr),
            size_(size){
            if(!(nodes_ = (CacheNode**)malloc(sizeof(CacheNode*)*size))){
                LOG(WARNING) << "cannot initialize memory pool cache of size: " << size;
                return;
            }
            memset(nodes_, 0, sizeof(Object*)*size);
        }
        ~MemoryPoolCache(){
            if(nodes_){
                for(size_t idx = 0; idx < size_; idx++){
                    CacheNode* node = nodes_[idx];
                    CacheNode* next = nullptr;
                    while(node != nullptr){
                        next = node->GetNext();
                        delete node;
                        node = next;
                    }
                }
                free(nodes_);
            }
        }

        size_t GetSize() const{
            return items_.size();
        }

        size_t GetMaxSize() const{
            return size_;
        }

        bool ContainsItem(const uint256_t& hash){
            return GetNode(hash) != nullptr;
        }

        bool RemoveItem(const uint256_t& hash){
            if(!ContainsItem(hash)) return false;
            std::deque<uint256_t>::iterator it = items_.begin();
            while((*it) != hash) it++;
            items_.erase(it);
            return RemoveNode(hash);
        }

        Object* GetItem(const uint256_t& hash){
            if(!ContainsItem(hash)) return nullptr;
            std::deque<uint256_t>::iterator it = items_.begin();
            while((*it) != hash) it++;
            items_.erase(it);
            items_.push_front(hash);
            return GetNode(hash)->GetValue();
        }

        bool PutItem(Object* value);
        bool Accept(ObjectPointerVisitor* vis);
    };
}

#endif //TOKEN_MEMORY_POOL_H