#ifndef TOKEN_POOL_H
#define TOKEN_POOL_H

#include <leveldb/db.h>
#include "common.h"
#include "crash_report.h"
#include "uint256_t.h"

namespace Token{
    class Block;
    class Transaction;
    class UnclaimedTransaction;

    template<typename PoolObject, typename RawType>
    class IndexManagedPool{
    private:
        std::string path_;
        leveldb::DB* index_;
        bool initialized_;

        typedef typename PoolObject::RawType RawPoolObject;
    protected:
        leveldb::DB* GetIndex() const{
            return index_;
        }

        inline std::string
        GetObjectLocation(const uint256_t& hash) const{
            leveldb::ReadOptions readOpts;
            std::string key = HexString(hash);
            std::string value;
            if(!GetIndex()->Get(readOpts, key, &value).ok()) return ""; // TODO: ???
            return value;
        }

        virtual std::string CreateObjectLocation(const uint256_t& hash, PoolObject* value) const = 0;

        bool InitializeIndex(){
            if(!FileExists(GetRoot())){
                if(!CreateDirectory(GetRoot())) return CrashReport::GenerateAndExit("Couldn't create directory: " + GetRoot());
            }
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

        PoolObject* LoadRawObject(const std::string& filename) const{
            std::fstream fd(filename, std::ios::in|std::ios::binary);
            RawPoolObject raw;
            if(!raw.ParseFromIstream(&fd)) return nullptr;
            return PoolObject::NewInstance(raw);
        }

        PoolObject* LoadObject(const uint256_t& hash) const{
            leveldb::ReadOptions readOpts;
            std::string key = HexString(hash);
            std::string location;
            if(!GetIndex()->Get(readOpts, key, &location).ok()) return nullptr;
            return LoadRawObject(location);
        }

        bool SaveRawObject(const std::string& filename, PoolObject* value) const{
            std::fstream fd(filename, std::ios::out|std::ios::binary);
            RawType raw;
            value->Encode(raw);
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
            return remove(filename.c_str()) == 0;
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