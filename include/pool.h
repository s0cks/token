#ifndef TOKEN_POOL_H
#define TOKEN_POOL_H

#include <leveldb/db.h>
#include "common.h"
#include "uint256_t.h"

namespace Token{
    template<typename PoolObject>
    class IndexManagedPool{
    private:
        std::string path_;
        leveldb::DB* index_;
        bool initialized_;
    protected:
        leveldb::DB* GetIndex() const{
            return index_;
        }

        virtual std::string CreateObjectLocation(const uint256_t& hash, PoolObject* value) const = 0;

        bool InitializeIndex(){
            leveldb::Options options;
            options.create_if_missing = true;
            return initialized_ = leveldb::DB::Open(options, GetRoot() + "/index", &index_).ok();
        }

        bool ContainsObject(const uint256_t& hash) const{
            leveldb::ReadOptions readOpts;
            std::string key = HexString(hash);
            std::string value;
            return GetIndex()->Get(readOpts, key, &value).ok();
        }

        bool LoadRawObject(const std::string& filename, PoolObject* result) const{
            std::fstream fd(filename, std::ios::in|std::ios::binary);
            typename PoolObject::RawType raw;
            if(!raw.ParseFromIstream(&fd)) return false;
            (*result) = PoolObject(raw);
            return true;
        }

        bool LoadObject(const uint256_t& hash, PoolObject* result) const{
            leveldb::ReadOptions readOpts;
            std::string key = HexString(hash);
            std::string location;
            if(!GetIndex()->Get(readOpts, key, &location).ok()) return false;
            return LoadRawObject(location, result);
        }

        bool SaveRawObject(const std::string& filename, PoolObject* value) const{
            std::fstream fd(filename, std::ios::out|std::ios::binary);
            typename PoolObject::RawType raw;
            raw << (*value);
            if(!raw.SerializeToOstream(&fd)) return false;
            return true;
        }

        bool SaveObject(const uint256_t& hash, PoolObject* value) const{
            if(ContainsObject(hash)) return false;
            leveldb::WriteOptions writeOpts;
            std::string key = HexString(hash);
            std::string location = CreateObjectLocation(hash, value);
            if(!GetIndex()->Put(writeOpts, key, location).ok()) return false;
            return SaveRawObject(location, value);
        }

        bool DeleteRawObject(const std::string& filename) const{
            return std::remove(filename.c_str()) == 0;
        }

        bool DeleteObject(const uint256_t& hash) const{
            std::string key = HexString(hash);
            std::string location;
            leveldb::ReadOptions readOpts;
            if(!GetIndex()->Get(readOpts, key, &location).ok()) return false;
            leveldb::WriteOptions writeOpts;
            if(!GetIndex()->Delete(writeOpts, key).ok()) return false;
            return DeleteRawObject(location);
        }

        bool IsInitialized() const{
            return initialized_;
        }

        IndexManagedPool(const std::string& path):
            path_(path),
            index_(nullptr),
            initialized_(false){}
    public:
        virtual ~IndexManagedPool() = default;

        std::string GetRoot() const{
            return path_;
        }
    };
}

#endif //TOKEN_POOL_H