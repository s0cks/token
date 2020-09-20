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

    Handle<Block> BlockChainIndex::GetBlockData(const uint256_t& hash){
        leveldb::ReadOptions options;
        std::string key = KEY(hash);
        std::string filename;

        LOCK_GUARD;
        if(!GetIndex()->Get(options, key, &filename).ok()){
            LOG(WARNING) << "couldn't find block " << hash << " in index";
            return nullptr;
        }

        Handle<Block> block = Block::NewInstance(filename);
        if(hash != block->GetHash()){
            LOG(WARNING) << "couldn't match block hashes: " << hash << " <=> " << block->GetHash();
            return nullptr;
        }

        return block;
    }

    void BlockChainIndex::PutReference(const std::string& name, const uint256_t& hash){
        leveldb::WriteOptions options;
        options.sync = true;

        LOCK_GUARD;
        if(!GetIndex()->Put(options, name, HexString(hash)).ok()){
            LOG(WARNING) << "couldn't put set reference " << name << " to : " << hash;
            return;
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
            LOG(WARNING) << "couldn't find reference: " << name;
            return uint256_t();
        }
        return HashFromHexString(value);
    }

    void BlockChainIndex::PutBlockData(const Handle<Block>& blk){
        BlockHeader block = blk->GetHeader();

        leveldb::WriteOptions options;
        options.sync = true;
        std::string key = KEY(block.GetHash());
        std::string filename = GetNewBlockFilename(blk);

        LOCK_GUARD;
        if(FileExists(filename)){
            LOG(WARNING) << "couldn't overwrite existing block data: " << filename;
            return;
        }

        if(!blk->WriteToFile(filename)){
            LOG(WARNING) << "couldn't write block " << block << " to file: " << filename;
            return;
        }

        if(!GetIndex()->Put(options, key, filename).ok()){
            LOG(WARNING) << "couldn't index block: " << block;
            return;
        }

        LOG(INFO) << "indexed block " << block << " to file: " << filename;
    }

    void BlockChainIndex::Initialize(){
        if(!FileExists(GetDataDirectory())){
            if(!CreateDirectory(GetDataDirectory())){
                LOG(WARNING) << "couldn't create block chain index in directory: " << GetDataDirectory();
                return;
            }
        }

        LOCK_GUARD;
        leveldb::Options options;
        options.create_if_missing = true;
        if(!leveldb::DB::Open(options, GetIndexFilename(), &index_).ok()){
            LOG(WARNING) << "couldn't initialize block chain index in file: " << GetIndexFilename();
            return;
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
            LOG(WARNING) << "couldn't list files in block chain index: " << GetDataDirectory();
            return 0;
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