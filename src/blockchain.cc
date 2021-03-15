#include <sstream>
#include <leveldb/db.h>
#include <glog/logging.h>

#include "blockchain.h"
#include "block_file.h"
#include "unclaimed_transaction.h"
#include "atomic/relaxed_atomic.h"
#include "blockchain_initializer.h"

namespace token{
  static inline std::string
  GetIndexFilename(){
    return GetBlockChainDirectory() + "/index";
  }

  static inline std::string
  GetNewBlockFilename(const BlockPtr& blk){
    std::stringstream ss;
    ss << GetBlockChainDirectory() << "/blk" << blk->GetHeight() << ".dat";
    return ss.str();
  }

  static inline std::string
  GetBlockHeightKey(int64_t height){
    std::stringstream ss;
    ss << "blk" << height;
    return ss.str();
  }

  //TODO: cleanup
  leveldb::Status BlockChain::InitializeIndex(const std::string& filename){
    if(!IsUninitialized()){
      LOG(WARNING) << "cannot reinitialize the block chain!";
      return leveldb::Status::NotSupported("Cannot re-initialize the block chain");
    }

#ifdef TOKEN_DEBUG
    LOG(INFO) << "initializing the block chain in " << filename << "....";
#endif//TOKEN_DEBUG
    if(!FileExists(filename)){
      LOG(INFO) << "creating " << filename << "....";
      if(!CreateDirectory(filename)){
        LOG(WARNING) << "couldn't create block chain index in directory: " << filename;
        return leveldb::Status::IOError("Cannot create the block chain index");
      }
    }

    leveldb::Options options;
    options.create_if_missing = true;
    options.comparator = new BlockChain::Comparator();

    leveldb::Status status;
    if(!(status = leveldb::DB::Open(options, GetIndexFilename(), &index_)).ok()){
      LOG(WARNING) << "couldn't initialize the block chain index in " << GetIndexFilename() << ": "
                   << status.ToString();
      return status;
    }

    if(!HasGenesis() || FLAGS_fresh){
      FreshBlockChainInitializer initializer(this);
      if(!initializer.InitializeBlockChain())
        return leveldb::Status::IOError("Cannot create a fresh block chain");
    } else{
      DefaultBlockChainInitializer initializer(this);
      if(!initializer.InitializeBlockChain())
        return leveldb::Status::IOError("Cannot initialize the block chain");
    }
    return leveldb::Status::OK();
  }

  static inline bool
  WriteBlockFile(const std::string& filename, const Hash& hash, const BlockPtr& blk){
    FILE* file;
    if(!(file = fopen(filename.data(), "wb"))){
      LOG(WARNING) << "cannot open file " << filename << ": " << strerror(errno);
      return false;
    }

    if(!WriteBlock(file, blk)){
      LOG(WARNING) << "cannot write block " << hash << " to file: " << filename;
      return false;
    }
    return Flush(file) && Close(file);
  }

  bool BlockChain::PutBlock(const Hash& hash, BlockPtr blk) const{
    BlockKey key(blk);
    std::string filename = GetNewBlockFilename(blk);

    leveldb::WriteOptions opts;
    opts.sync = true;

    leveldb::Status status;
    if(!(status = GetIndex()->Put(opts, key, filename)).ok()){
      LOG(WARNING) << "cannot index object " << hash << ": " << status.ToString();
      return false;
    }

    if(!WriteBlockFile(filename, hash, blk)){
      LOG(ERROR) << "cannot write block file";
      return false;//TODO: better error handling
    }

    LOG(INFO) << "indexed block: " << hash;
    return true;
  }

  static inline BlockPtr
  ReadBlockFile(const std::string& filename, const Hash& hash){
    FILE* file;
    if(!(file = fopen(filename.data(), "rb"))){
      LOG(ERROR) << "cannot open file " << filename << ": " << strerror(errno);
      return BlockPtr(nullptr);
    }

    BlockPtr blk = ReadBlock(file);
    if(!blk){
      LOG(ERROR) << "cannot read block " << hash << " from file: " << filename;
      if(!Close(file))
        LOG(ERROR) << "cannot close block file: " << filename;
      return BlockPtr(nullptr);
    }
    return blk;
  }

  BlockPtr BlockChain::GetBlock(const Hash& hash) const{
    BlockKey key(0, hash);

    std::string filename;
    leveldb::Status status;
    if(!(status = GetIndex()->Get(leveldb::ReadOptions(), key, &filename)).ok()){
      LOG(WARNING) << "cannot get " << hash << ": " << status.ToString();
      return BlockPtr(nullptr);
    }
    return ReadBlockFile(filename, hash);
  }

  BlockPtr BlockChain::GetBlock(const int64_t& height) const{
    //TODO: implement
    return BlockPtr(nullptr);
  }

  bool BlockChain::HasReference(const std::string& name) const{
    ReferenceKey key(name);
    std::string value;
    return GetIndex()->Get(leveldb::ReadOptions(), key, &value).ok();
  }

  bool BlockChain::RemoveBlock(const Hash& hash, const BlockPtr& blk) const{
    leveldb::WriteOptions options;
    options.sync = true;

    BlockKey key(blk);

    leveldb::Status status;
    if(!(status = GetIndex()->Delete(options, key)).ok()){
      LOG(WARNING) << "couldn't remove block " << hash << ": " << status.ToString();
      return false;
    }

    LOG(INFO) << "removed block: " << hash;
    return true;
  }

  bool BlockChain::RemoveReference(const std::string& name) const{
    leveldb::WriteOptions options;
    options.sync = true;

    ReferenceKey key(name);
    std::string value;

    leveldb::Status status;
    if(!(status = GetIndex()->Delete(options, key)).ok()){
      LOG(WARNING) << "couldn't remove reference " << name << ": " << status.ToString();
      return false;
    }

    LOG(INFO) << "removed reference: " << name;
    return true;
  }

  bool BlockChain::PutReference(const std::string& name, const Hash& hash) const{
    leveldb::WriteOptions options;
    options.sync = true;

    ReferenceKey key(name);
    std::string value = hash.HexString();

    leveldb::Status status;
    if(!(status = GetIndex()->Put(options, key, value)).ok()){
      LOG(WARNING) << "couldn't set reference " << name << " to " << hash << ": " << status.ToString();
      return false;
    }

    LOG(INFO) << "set reference " << name << " := " << hash;
    return true;
  }

  Hash BlockChain::GetReference(const std::string& name) const{
    ReferenceKey key(name);
    std::string value;

    leveldb::Status status;
    if(!(status = GetIndex()->Get(leveldb::ReadOptions(), key, &value)).ok()){
      LOG(WARNING) << "couldn't find reference " << name << ": " << status.ToString();
      return Hash();
    }

    return Hash::FromHexString(value);
  }

  bool BlockChain::HasBlock(const Hash& hash) const{
    BlockKey key(0, hash);

    std::string filename;
    leveldb::Status status;
    if(!(status = GetIndex()->Get(leveldb::ReadOptions(), key, &filename)).ok()){
      if(status.IsNotFound()){
#ifdef TOKEN_DEBUG
        LOG(WARNING) << "couldn't find block '" << hash << "'";
#endif//TOKEN_DEBUG
        return false;
      }

#ifdef TOKEN_DEBUG
      LOG(ERROR) << "error getting block " << hash << ": " << status.ToString();
#endif//TOKEN_DEBUG
      return false;
    }

    LOG(INFO) << "found " << hash << ": " << status.ToString();
    return true;
  }

  bool BlockChain::Append(const BlockPtr& block){
    BlockPtr head = GetHead();
    Hash hash = block->GetHash();
    Hash phash = block->GetPreviousHash();

    LOG(INFO) << "appending new block:";
    if(HasBlock(hash)){
      LOG(WARNING) << "duplicate block found for: " << hash;
      return false;
    }

    if(block->IsGenesis()){
      LOG(WARNING) << "cannot append genesis block: " << hash;
      return false;
    }

    if(!HasBlock(phash)){
      LOG(WARNING) << "cannot find parent block: " << phash;
      return false;
    }

    PutBlock(hash, block);
    if(head->GetHeight() < block->GetHeight())
      PutReference(BLOCKCHAIN_REFERENCE_HEAD, hash);
    return true;
  }

  bool BlockChain::VisitBlocks(BlockChainBlockVisitor* vis) const{
    Hash current = GetReference(BLOCKCHAIN_REFERENCE_HEAD);
    do{
      BlockPtr blk = GetBlock(current);
      if(!vis->Visit(blk))
        return false;
      current = blk->GetPreviousHash();
    } while(!current.IsNull());
    return true;
  }

  static inline bool
  IsValidBlock(const std::string& filename){
    //TODO: implement
    LOG(ERROR) << "not implemented yet.";
    return false;
  }

  int64_t BlockChain::GetNumberOfBlocks() const{
    std::string home = GetBlockChainDirectory();

    int64_t count = 0;

    DIR* dir;
    struct dirent* ent;
    if((dir = opendir(home.c_str())) != NULL){
      while((ent = readdir(dir)) != NULL){
        std::string filename(ent->d_name);
        std::string abs_path = home + '/' + filename;
        if(EndsWith(filename, ".dat") && IsValidBlock(abs_path)){
          count++;
        }
      }
      closedir(dir);
    } else{
      LOG(WARNING) << "couldn't list files in block chain index: " << home;
      return 0;
    }
    return count;
  }

  bool BlockChain::GetBlocks(Json::Writer& writer) const{
    writer.StartArray();
    {
      BlockPtr current = GetHead();
      do{
        Json::Append(writer, current->GetHash());
        current = GetBlock(current->GetPreviousHash());
      } while(current);
    }
    writer.EndArray();
    return true;
  }

  static BlockChain instance;
  static JobQueue queue_(JobScheduler::kMaxNumberOfJobs);

  bool BlockChain::Initialize(const std::string& filename){
    if(!JobScheduler::RegisterQueue(GetCurrentThread(), &queue_)){
      LOG(ERROR) << "couldn't register BlockChain job queue.";
      return false;
    }

    leveldb::Status status;
    if(!(status = instance.InitializeIndex(filename)).ok()){
      LOG(ERROR) << "couldn't initialize the BlockChain in " << filename << ": " << status.ToString();
      return false;
    }
    return true;
  }

  BlockChain* BlockChain::GetInstance(){
    return &instance;
  }
}