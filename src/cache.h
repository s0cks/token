#ifndef TOKEN_CACHE_H
#define TOKEN_CACHE_H

#include <map>
#include <deque>

#include "node.h"
#include "handle.h"
#include "uint256_t.h"

namespace Token{
    template<typename T>
    class CacheVisitor{
    protected:
        CacheVisitor() = default;
    public:
        virtual ~CacheVisitor() = default;
        virtual bool Visit(const Handle<T>& value) = 0;
    };

    template<typename T, size_t CacheSize>
    class Cache{
        class CacheNode{
        protected:
            CacheNode* previous_;
            CacheNode* next_;
            uint256_t hash_;
            T* data_;

            void WriteBarrier(T** slot, T* value){
                (*slot) = value;
            }

            inline void
            WriteBarrier(T** slot, const Handle<T>& value){
                WriteBarrier(slot, (T*)value);
            }
        public:
            CacheNode(const uint256_t& key, T* data):
                previous_(nullptr),
                next_(nullptr),
                hash_(key),
                data_(nullptr){
                WriteBarrier(&data_, data);
            }
            ~CacheNode() = default;

            CacheNode* GetNext() const{
                return next_;
            }

            bool HasNext() const{
                return next_ != nullptr;
            }

            void SetNext(CacheNode* node){
                next_ = node;
            }

            CacheNode* GetPrevious() const{
                return previous_;
            }

            bool HasPrevious() const{
                return previous_;
            }

            void SetPrevious(CacheNode* node){
                previous_ = node;
            }

            uint256_t GetKey() const{
                return hash_;
            }

            Handle<T> GetValue() const{
                return data_;
            }

            void SetValue(T* value){
                data_ = value;
            }

            void SetValue(const Handle<T>& value){
                SetValue((T*)value);
            }

            bool Accept(WeakReferenceVisitor* vis){
                return vis->Visit(&data_);
            }

            bool Accept(ObjectPointerVisitor* vis){
                return vis->Visit(data_);
            }
        };
    private:
        size_t size_;
        CacheNode* buckets_[CacheSize];
        std::deque<uint256_t> cache_items_;

        CacheNode* GetNode(const uint256_t& hash){
            unsigned long bucket = hash.GetHashCode() % CacheSize;
            CacheNode* entry = buckets_[bucket];
            while(entry != nullptr){
                if(entry->GetKey() == hash) return entry;
                entry = entry->GetNext();
            }
            return nullptr;
        }

        bool PutNode(const uint256_t& hash, const Handle<T>& value){
            unsigned long bucket = hash.GetHashCode() % CacheSize;
            CacheNode* prev = nullptr;
            CacheNode* entry = buckets_[bucket];
            while(entry != nullptr && entry->GetKey() != hash){
                prev = entry;
                entry = entry->GetNext();
            }

            if(entry == nullptr){
                entry = new CacheNode(hash, (T*)value);
                if(prev == nullptr){
                    buckets_[bucket] = entry;
                } else{
                    prev->SetNext(entry);
                }
            } else{
                entry->SetValue(value);
            }
            return true;
        }

        bool RemoveNode(const uint256_t& hash){
            unsigned long bucket = hash.GetHashCode() % CacheSize;
            CacheNode* prev = nullptr;
            CacheNode* entry = buckets_[bucket];
            while(entry != nullptr && entry->GetKey() != hash){
                prev = entry;
                entry = entry->GetNext();
            }

            if(entry == nullptr) return false;
            if(prev == nullptr){
                buckets_[bucket] = entry->GetNext();
            } else{
                prev->SetNext(entry->GetNext());
            }

            delete entry;
            return true;
        }

        inline void
        EvictLeastUsed(){
            uint256_t last = cache_items_.back();
            cache_items_.pop_back();
            RemoveNode(last);
            LOG(INFO) << last << " evicted from cache";
        }

        inline void
        RemoveItem(const uint256_t& hash){
            std::deque<uint256_t>::iterator it = cache_items_.begin();
            while((*it) != hash) it++;
            cache_items_.erase(it);
            RemoveNode(hash);
        }
    public:
        Cache():
            buckets_(),
            cache_items_(){
            memset(buckets_, 0, sizeof(buckets_));
        }
        ~Cache() = default;

        size_t GetSize() const{
            return cache_items_.size();
        }

        size_t GetMaxSize() const{
            return size_;
        }

        bool Contains(const uint256_t& hash){
            return GetNode(hash) != nullptr;
        }

        bool Remove(const uint256_t& hash){
            if(!Contains(hash)) return false;
            RemoveItem(hash);
            return true;
        }

        bool Put(const uint256_t& hash, const Handle<T>& value){
            if(!Contains(hash)){
                if(GetSize() > GetMaxSize())
                    EvictLeastUsed();
            } else{
                RemoveItem(hash);
            }

            cache_items_.push_front(hash);
            return PutNode(hash, value);
        }

        Handle<T> Get(const uint256_t& hash){
            if(!Contains(hash)) return nullptr;
            std::deque<uint256_t>::iterator it = cache_items_.begin();
            while((*it) != hash) it++;
            cache_items_.erase(it);
            cache_items_.push_front(hash);
            return GetNode(hash)->GetValue();
        }

        bool Accept(ObjectPointerVisitor* vis){
            for(unsigned long idx = 0;
                idx < CacheSize;
                idx++){
                CacheNode* bucket = buckets_[idx];
                while(bucket != nullptr){
                    if(!bucket->Accept(vis))
                        return false;
                    bucket = bucket->GetNext();
                }
            }
            return true;
        }

        bool Accept(WeakReferenceVisitor* vis){
            for(unsigned long idx = 0;
                idx < size_;
                idx++){
                CacheNode* bucket = buckets_[idx];
                while(bucket != nullptr){
                    if(!bucket->Accept(vis))
                        return false;
                    bucket = bucket->GetNext();
                }
            }
            return true;
        }
    };
}

#endif //TOKEN_CACHE_H