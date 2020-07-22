#include <glog/logging.h>
#include <leveldb/db.h>
#include "crash_report.h"
#include "unclaimed_transaction_pool_index.h"

namespace Token{
    static leveldb::DB* index_ = nullptr;

#define KEY(Hash) (HexString((Hash)))

    static inline leveldb::DB*
    GetIndex(){
        return index_;
    }

    static inline std::string
    GetDataDirectory(){
        return FLAGS_path + "/utxos";
    }

    static inline std::string
    GetIndexFilename(){
        return GetDataDirectory() + "/index";
    }

    static inline std::string
    GetNewDataFilename(const uint256_t& hash){
        std::string hashString = HexString(hash);
        std::string front = hashString.substr(0, 8);
        std::string tail = hashString.substr(hashString.length() - 8, hashString.length());
        std::string filename = GetDataDirectory() + "/" + front + ".dat";
        if(FileExists(filename)){
            filename = GetDataDirectory() + "/" + tail + ".dat";
        }
        return filename;
    }

    static inline std::string
    GetDataFilename(const uint256_t& hash){
        leveldb::ReadOptions options;
        std::string key = HexString(hash);
        std::string filename;
        if(!GetIndex()->Get(options, key, &filename).ok()) return GetNewDataFilename(hash);
        return filename;
    }

    std::string UnclaimedTransactionPoolIndex::GetDirectory(){
        return GetDataDirectory();
    }

    void UnclaimedTransactionPoolIndex::Initialize(){
        if(!FileExists(GetDataDirectory())){
            if(!CreateDirectory(GetDataDirectory())){
                std::stringstream ss;
                ss << "Couldn't create transaction pool cache in directory: " << GetDataDirectory();
                CrashReport::GenerateAndExit(ss);
            }
        }

        leveldb::Options options;
        options.create_if_missing = true;
        if(!leveldb::DB::Open(options, GetIndexFilename(), &index_).ok()){
            std::stringstream ss;
            ss << "Couldn't initialize the transaction pool cache in file: " << GetIndexFilename();
            CrashReport::GenerateAndExit(ss);
        }
    }

    bool UnclaimedTransactionPoolIndex::PutData(const Handle<UnclaimedTransaction>& tx){
        leveldb::WriteOptions options;
        options.sync = true;
        std::string key = KEY(tx->GetSHA256Hash());
        std::string filename = GetNewDataFilename(tx->GetSHA256Hash());

        if(FileExists(filename)){
            LOG(WARNING) << "couldn't overwrite existing unclaimed transaction data: " << filename;
            return false;
        }

        std::fstream fd(filename, std::ios::out|std::ios::binary);
        if(!tx->WriteToFile(fd)){
            LOG(WARNING) << "couldn't write unclaimed transaction " << tx->GetSHA256Hash() << " to file: " << filename;
            return false;
        }

        if(!GetIndex()->Put(options, key, filename).ok()){
            LOG(WARNING) << "couldn't index unclaimed transaction: " << tx->GetSHA256Hash();
            return false;
        }
    }

    size_t UnclaimedTransactionPoolIndex::GetNumberOfUnclaimedTransactions(){
        uint32_t count = 0;

        DIR* dir;
        struct dirent* ent;
        if((dir = opendir(GetDataDirectory().c_str())) != NULL){
            while((ent = readdir(dir)) != NULL){
                std::string filename(ent->d_name);
                if(EndsWith(filename, ".dat")) count++;
            }
            closedir(dir);
        } else{
            std::stringstream ss;
            ss << "Couldn't list files in transaction pool cache directory: " << GetDataDirectory();
            CrashReport::GenerateAndExit(ss);
        }

        return count;
    }

    Handle<UnclaimedTransaction> UnclaimedTransactionPoolIndex::GetData(const uint256_t& hash){
        leveldb::ReadOptions options;
        std::string key = KEY(hash);
        std::string filename;
        if(!GetIndex()->Get(options, key, &filename).ok()){
            std::stringstream ss;
            ss << "Couldn't find transaction " << hash << " in index";
            CrashReport::GenerateAndExit(ss);
        }
        return UnclaimedTransaction::NewInstance(filename);
    }

    bool UnclaimedTransactionPoolIndex::HasData(const uint256_t& hash){
        leveldb::ReadOptions options;
        std::string key = KEY(hash);
        std::string filename;
        return GetIndex()->Get(options, key, &filename).ok();
    }
}