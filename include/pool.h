#ifndef TOKEN_POOL_H
#define TOKEN_POOL_H

#include <leveldb/db.h>
#include "common.h"
#include "uint256_t.h"

namespace Token{
    class IndexManagedPool{
    private:
        std::string path_;
        leveldb::DB* index_;
        bool initialized_;
    protected:
        leveldb::DB* GetIndex() const{
            return index_;
        }

        bool InitializeIndex();

        bool IsInitialized(){
            return initialized_;
        }

        IndexManagedPool(const std::string& path):
            path_(path),
            index_(nullptr),
            initialized_(false){}
    public:
        virtual ~IndexManagedPool() = default;

        std::string GetRoot(){
            return path_;
        }

        uint64_t GetSize();
        std::string GetPath(const uint256_t& hash);
        bool RegisterPath(const uint256_t& hash, const std::string& filename);
        bool UnregisterPath(const uint256_t& hash);
    };
}

#endif //TOKEN_POOL_H