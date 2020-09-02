#ifndef TOKEN_OBJECT_CACHE_H
#define TOKEN_OBJECT_CACHE_H

#include <leveldb/db.h>
#include "handle.h"
#include "uint256_t.h"

namespace Token{
    template<typename T>
    class ObjectCacheVisitor{
    protected:
        ObjectCacheVisitor() = default;
    public:
        virtual ~ObjectCacheVisitor() = default;
        virtual bool Visit(const Handle<T>& value) = 0;
    };

    template<typename T>
    class ObjectCache{
        //TODO: refactor this, bad implementation
    protected:
        leveldb::DB* index_;
        std::string path_;

        leveldb::DB* GetIndex() const{
            return index_;
        }

        std::string GetIndexFilename() const{
            return GetPath() + "/index";
        }

        std::string GetNewDataFilename(const uint256_t& hash){
            std::string hashString = HexString(hash);
            std::string front = hashString.substr(0, 8);
            std::string tail = hashString.substr(hashString.length() - 8, hashString.length());
            std::string filename = GetPath() + "/" + front + ".dat";
            if(FileExists(filename)){
                filename = GetPath() + "/" + tail + ".dat";
            }
            return filename;
        }

        std::string GetDataFilename(const uint256_t& hash){
            leveldb::ReadOptions options;
            std::string key = HexString(hash);
            std::string filename;
            return GetIndex()->Get(options, key, &filename).ok() ?
                    filename :
                    GetNewDataFilename(hash);
        }

        bool Initialize(){
            if(!FileExists(GetPath())){
                if(!CreateDirectory(GetPath())){
                    LOG(WARNING) << "couldn't create cache in: " << GetPath();
                    return false;
                }
            }

            leveldb::Options options;
            options.create_if_missing = true;
            if(!leveldb::DB::Open(options, GetIndexFilename(), &index_).ok()){
                LOG(WARNING) << "couldn't initialize cache in file: " << GetIndexFilename();
                return false;
            }
            return true;
        }
    public:
        ObjectCache(const std::string& path):
            index_(nullptr),
            path_(path){
            if(!Initialize()) LOG(WARNING) << "couldn't initialize object cache in directory: " << path;
        }
        ~ObjectCache(){
            if(index_) delete index_;
        }

        std::string GetPath() const{
            return path_;
        }

        bool IsInitialized() const{
            return index_ != nullptr;
        }

        bool HasData() const{
            return GetNumberOfObjects() > 0;
        }

        bool HasData(const uint256_t& hash) const{
            leveldb::ReadOptions options;
            std::string key = HexString(hash);
            std::string filename;
            return GetIndex()->Get(options, key, &filename).ok();
        }

        bool PutData(const Handle<T>& data){
            leveldb::WriteOptions options;
            options.sync = true;
            std::string key = HexString(data->GetHash());
            std::string filename = GetNewDataFilename(data->GetHash());

            if(FileExists(filename)){
                LOG(WARNING) << "cannot overwrite existing data in cache: " << filename;
                return false;
            }

            std::fstream fd(filename, std::ios::out|std::ios::binary);
            if(!data->WriteToFile(fd)){
                LOG(WARNING) << "couldn't write object data to cache file: " << filename;
                return false;
            }

            if(!GetIndex()->Put(options, key, filename).ok()){
                LOG(WARNING) << "couldn't index the following object in the cache: " << data->GetHash();
                return false;
            }
            return true;
        }

        bool RemoveData(const uint256_t& hash){
            if(!HasData(hash)) return false;
            leveldb::WriteOptions options;
            options.sync = true;
            std::string key = HexString(hash);
            std::string filename = GetDataFilename(hash);
            if(!GetIndex()->Delete(options, key).ok()){
                LOG(WARNING) << "couldn't remove object from cache index: " << hash;
                return false;
            }
            if(!DeleteFile(filename)){
                LOG(WARNING) << "couldn't delete cache object data: " << filename;
                return false;
            }
            return true;
        }

        Handle<T> GetData(const uint256_t& hash) const{
            leveldb::ReadOptions options;
            std::string key = HexString(hash);
            std::string filename;
            if(!GetIndex()->Get(options, key, &filename).ok()){
                LOG(WARNING) << "couldn't find object in cache: " << hash;
                return nullptr;
            }
            return T::NewInstance(filename);
        }

        size_t GetNumberOfObjects() const{
            uint32_t count = 0;

            DIR* dir;
            struct dirent* ent;
            if((dir = opendir(GetPath().c_str())) != NULL){
                while((ent = readdir(dir)) != NULL){
                    std::string filename(ent->d_name);
                    if(EndsWith(filename, ".dat")) count++;
                }
                closedir(dir);
            } else{
                LOG(WARNING) << "couldn't list files in object cache directory: " << GetPath();
                return false;
            }
            return count;
        }

        bool Accept(ObjectCacheVisitor<T>* vis){
            DIR* dir;
            struct dirent* ent;
            if((dir = opendir(GetPath().c_str())) != NULL){
                while((ent = readdir(dir)) != NULL){
                    std::string name(ent->d_name);
                    std::string filename = (GetPath() + "/" + name);
                    if(!EndsWith(filename, ".dat")) continue;

                    Handle<T> data = T::NewInstance(filename);
                    if(!vis->Visit(data)) return false;
                }

                closedir(dir);
                return true;
            }
            return false;
        }
    };
}

#endif //TOKEN_OBJECT_CACHE_H