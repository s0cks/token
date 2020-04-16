#include <glog/logging.h>
#include <dirent.h>
#include "pool.h"

namespace Token{
    bool IndexManagedPool::InitializeIndex(){
        leveldb::Options options;
        options.create_if_missing = true;
        return initialized_ = leveldb::DB::Open(options, GetRoot() + "/index", &index_).ok();
    }

    uint64_t IndexManagedPool::GetSize(){
        uint64_t count = 0;

        DIR* dir;
        struct dirent* ent;
        if((dir = opendir(GetRoot().c_str())) != NULL){
            while((ent = readdir(dir)) != NULL){
                std::string filename(ent->d_name);
                if(EndsWith(filename, ".dat")) count++;
            }
        }
        return count;
    }

    std::string IndexManagedPool::GetPath(const uint256_t& hash){
        leveldb::ReadOptions readOpts;
        std::string key = HexString(hash);
        std::string value;
        if(GetIndex()->Get(readOpts, key, &value).ok()) return value;
        std::stringstream filename;
        filename << GetRoot() << "/" << (GetSize()) << ".dat";
        return filename.str();
    }

    bool IndexManagedPool::RegisterPath(const uint256_t& hash, const std::string& filename){
        leveldb::WriteOptions writeOpts;
        std::string key = HexString(hash);
        return GetIndex()->Put(writeOpts, key, filename).ok();
    }

    bool IndexManagedPool::UnregisterPath(const uint256_t& hash){
        leveldb::WriteOptions writeOpts;
        std::string key = HexString(hash);
        return GetIndex()->Delete(writeOpts, key).ok();
    }
}