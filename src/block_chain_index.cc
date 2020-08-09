#include "block_chain_index.h"
#include "crash_report.h"

namespace Token{
    static std::recursive_mutex mutex_;
    static leveldb::DB* index_ = nullptr;

#define LOCK_GUARD std::lock_guard<std::recursive_mutex> guard(mutex_)
#define KEY(Hash) HexString((Hash))

    static inline std::string
    GetDataDirectory(){
        return FLAGS_path + "/data";
    }

    static inline std::string
    GetIndexFilename(){
        return GetDataDirectory() + "/index";
    }

    static inline std::string
    GetNewBlockFilename(Block* blk){
        std::stringstream ss;
        ss << GetDataDirectory() + "/blk" << blk->GetHeight() << ".dat";
        return ss.str();
    }

    leveldb::DB* BlockChainIndex::GetIndex(){
        return index_;
    }

    Block* BlockChainIndex::GetBlockData(const uint256_t& hash){
        leveldb::ReadOptions options;
        std::string key = KEY(hash);
        std::string filename;

        LOCK_GUARD;
        if(!GetIndex()->Get(options, key, &filename).ok()){
            std::stringstream ss;
            ss << "Couldn't find block " << hash << " in index";
            CrashReport::GenerateAndExit(ss);
        }

        Block* block = Block::NewInstance(filename);
        if(hash != block->GetSHA256Hash()){
            std::stringstream ss;
            ss << "Couldn't match block hash";
            ss << std::endl;
            ss << "\tExpected Hash: " << hash << std::endl;
            ss << "\tBlock Information: " << std::endl;
            ss << "\t  - Timestamp: " << block->GetTimestamp() << std::endl;
            ss << "\t  - Height: " << block->GetHeight() << std::endl;
            ss << "\t  - Previous Hash: " << block->GetPreviousHash() << std::endl;
            ss << "\t  - Merkle Root: " << block->GetMerkleRoot() << std::endl;
            ss << "\t  - Hash: " << block->GetSHA256Hash() << std::endl;
            ss << "\t  - Number of Transactions: " << block->GetNumberOfTransactions() << std::endl;
            CrashReport::GenerateAndExit(ss);
        }

        return block;
    }

    void BlockChainIndex::PutReference(const std::string& name, const uint256_t& hash){
        leveldb::WriteOptions options;
        options.sync = true;

        LOCK_GUARD;
        if(!GetIndex()->Put(options, name, HexString(hash)).ok()){
            std::stringstream ss;
            ss << "Couldn't put set reference " << name << " to : " << hash;
            CrashReport::GenerateAndExit(ss);
        }

#ifdef TOKEN_DEBUG
        LOG(INFO) << "set reference: " << name << " := " << hash;
#endif//TOKEN_DEBUG
    }

    bool BlockChainIndex::HasReference(const std::string& name){
        leveldb::ReadOptions options;
        std::string value;
        LOCK_GUARD;
        return GetIndex()->Get(options, name, &value).ok();
    }

    uint256_t BlockChainIndex::GetReference(const std::string& name){
        leveldb::ReadOptions options;
        std::string value;
        LOCK_GUARD;
        if(!GetIndex()->Get(options, name, &value).ok()){
            std::stringstream ss;
            ss << "Couldn't find reference: " << name;
            CrashReport::GenerateAndExit(ss);
        }
        return HashFromHexString(value);
    }

    void BlockChainIndex::PutBlockData(Block* blk){
        BlockHeader block = blk->GetHeader();

        leveldb::WriteOptions options;
        options.sync = true;
        std::string key = KEY(block.GetHash());
        std::string filename = GetNewBlockFilename(blk);

        LOCK_GUARD;
        if(FileExists(filename)){
            std::stringstream ss;
            ss << "Couldn't overwrite existing block data: " << filename;
            CrashReport::GenerateAndExit(ss);
        }

        std::fstream fd(filename, std::ios::out|std::ios::binary);
        /*
        if(!blk->WriteToFile(fd)){
            std::stringstream ss;
            ss << "Couldn't write block " << block << " to file: " << filename;
            CrashReport::GenerateAndExit(ss);
        }
        */

        if(!GetIndex()->Put(options, key, filename).ok()){
            std::stringstream ss;
            ss << "Couldn't index block: " << block;
            CrashReport::GenerateAndExit(ss);
        }

#ifdef TOKEN_DEBUG
        LOG(INFO) << "indexed block " << block << " to file: " << filename;
#endif//TOKEN_DEBUG
    }

    void BlockChainIndex::Initialize(){
        if(!FileExists(GetDataDirectory())){
            if(!CreateDirectory(GetDataDirectory())){
                std::stringstream ss;
                ss << "Couldn't create block chain index in directory: " << GetDataDirectory();
                CrashReport::GenerateAndExit(ss);
            }
        }

        LOCK_GUARD;
        leveldb::Options options;
        options.create_if_missing = true;
        if(!leveldb::DB::Open(options, GetIndexFilename(), &index_).ok()){
            std::stringstream ss;
            ss << "Couldn't initialize block chain index in file: " << GetIndexFilename();
            CrashReport::GenerateAndExit(ss);
        }
    }

    uint32_t BlockChainIndex::GetNumberOfBlocks(){
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
            ss << "Couldn't list files in block chain index: " << GetDataDirectory();
            CrashReport::GenerateAndExit(ss);
        }

        return count;
    }

    bool BlockChainIndex::HasBlockData(const uint256_t& hash){
        leveldb::ReadOptions options;
        std::string key = KEY(hash);
        std::string filename;

        LOCK_GUARD;
        return GetIndex()->Get(options, key, &filename).ok();
    }
}