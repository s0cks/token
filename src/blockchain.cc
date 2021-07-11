#include <sstream>
#include <leveldb/db.h>
#include <glog/logging.h>

#include "buffer.h"
#include "reference.h"
#include "filesystem.h"
#include "blockchain.h"
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
    ss << GetBlockChainDirectory() << "/blk" << blk->height() << ".dat";
    return ss.str();
  }

  static inline std::string
  GetBlockHeightKey(int64_t height){
    std::stringstream ss;
    ss << "blk" << height;
    return ss.str();
  }

  static inline bool
  ShouldCreateFreshInstall(){
    return FLAGS_reinitialize || !config::HasProperty(BLOCKCHAIN_REFERENCE_HEAD);
  }

  //TODO: cleanup
  leveldb::Status BlockChain::InitializeIndex(const std::string& filename){
    if(!IsUninitialized())
      return leveldb::Status::NotSupported("cannot re-initialize the block chain");

    DLOG_CHAIN(INFO) << "initializing in " << filename << "....";
    if(!FileExists(filename)){
      DLOG_CHAIN(INFO) << "creating " << filename << "....";
      if(!CreateDirectory(filename)){
        std::stringstream ss;
        ss << "couldn't create the block chain directory: " << filename;
        return leveldb::Status::IOError(ss.str());
      }
    }

    leveldb::Options options;
    options.create_if_missing = true;
    options.comparator = new BlockChain::Comparator();

    leveldb::Status status;
    if(!(status = leveldb::DB::Open(options, GetIndexFilename(), &index_)).ok()){
      DLOG_CHAIN(WARNING) << "couldn't initialize the index: " << status.ToString();
      return status;
    }

    if(ShouldCreateFreshInstall()){
      FreshBlockChainInitializer initializer(shared_from_this());
      if(!initializer.InitializeBlockChain())
        return leveldb::Status::IOError("Cannot create a fresh block chain");
    } else{
      DefaultBlockChainInitializer initializer(shared_from_this());
      if(!initializer.InitializeBlockChain())
        return leveldb::Status::IOError("Cannot initialize the block chain");
    }
    return leveldb::Status::OK();
  }

  //TODO: refactor
  static inline bool
  WriteBlockFile(const std::string& filename, const Hash& hash, const BlockPtr& blk){
    FILE* file;
    if(!(file = fopen(filename.data(), "wb"))){
      LOG(WARNING) << "cannot open file " << filename << ": " << strerror(errno);
      return false;
    }

    Block::Encoder encoder(*blk, codec::EncodeVersionFlag::Encode(true));
    BufferPtr buffer = internal::NewBuffer(encoder.GetBufferSize());
    if(!encoder.Encode(buffer)){
      LOG(ERROR) << "cannot encode Block " << blk->hash() << ".";
      return false;
    }

    if(!buffer->WriteTo(file)){
      LOG(ERROR) << "couldn't write Block to file: " << filename;
      return false;
    }

    return internal::Flush(file)
        && internal::Close(file);
  }

  bool BlockChain::PutBlock(const Hash& hash, const BlockPtr& blk) const{
    ObjectKey key(Type::kBlock, hash);
    std::string filename = GetNewBlockFilename(blk);

    leveldb::WriteOptions opts;
    opts.sync = true;

    leveldb::Status status;
    if(!(status = GetIndex()->Put(opts, (const leveldb::Slice&)key, filename)).ok()){
      LOG_CHAIN(WARNING) << "cannot index object " << hash << ": " << status.ToString();
      return false;
    }

    if(!WriteBlockFile(filename, hash, blk)){
      LOG_CHAIN(ERROR) << "cannot write block file";
      return false;//TODO: better error handling
    }

    DLOG_CHAIN(INFO) << "indexed block: " << hash;
    return true;
  }

  static inline BlockPtr
  ReadBlockFile(const std::string& filename, const Hash& hash){
    BufferPtr buffer = internal::NewBufferFromFile(filename);
    BlockPtr block = Block::DecodeNew(buffer, codec::ExpectTypeHint::Encode(true)|codec::ExpectVersionHint::Encode(true));
    LOG_IF(FATAL, block->hash() != hash) << "expected block hash of " << hash << ", but parsed block was: " << block->ToString();
    return block;
  }

  BlockPtr BlockChain::GetBlock(const Hash& hash) const{
    ObjectKey key(Type::kBlock, hash);

    std::string filename;
    leveldb::Status status;
    if(!(status = GetIndex()->Get(leveldb::ReadOptions(), (const leveldb::Slice&)key, &filename)).ok()){
      if(status.IsNotFound()){
        DLOG_CHAIN(WARNING) << "couldn't find block: " << hash;
        return nullptr;
      }

      LOG_CHAIN(ERROR) << "cannot get block " << hash << ": " << status.ToString();
      return nullptr;
    }
    return ReadBlockFile(filename, hash);
  }

  BlockPtr BlockChain::GetBlock(const int64_t& height) const{
    //TODO: implement
    return BlockPtr(nullptr);
  }

  bool BlockChain::RemoveBlock(const Hash& hash, const BlockPtr& blk) const{
    leveldb::WriteOptions options;
    options.sync = true;

    ObjectKey key(Type::kBlock, hash);

    leveldb::Status status;
    if(!(status = GetIndex()->Delete(options, (const leveldb::Slice&)key)).ok()){
      LOG_CHAIN(ERROR) << "couldn't remove block " << hash << ": " << status.ToString();
      return false;
    }

    DLOG_CHAIN(INFO) << "removed block: " << hash;
    return true;
  }

  bool BlockChain::HasBlock(const Hash& hash) const{
    ObjectKey key(Type::kBlock, hash);

    std::string filename;
    leveldb::Status status;
    if(!(status = GetIndex()->Get(leveldb::ReadOptions(), (const leveldb::Slice&)key, &filename)).ok()){
      if(status.IsNotFound())
        return false;

      LOG_CHAIN(ERROR) << "couldn't get block " << hash << ": " << status.ToString();
      return false;
    }
    return true;
  }

  //TODO: cleanup logging
  bool BlockChain::Append(const BlockPtr& block){
    BlockPtr head = GetHead();
    Hash hash = block->hash();
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
    if(head->height() < block->height()){
      if(!config::PutProperty(BLOCKCHAIN_REFERENCE_HEAD, hash))
        return false;
    }
    return true;
  }

  bool BlockChain::VisitBlocks(BlockChainBlockVisitor* vis) const{
    Hash current = config::GetHash(BLOCKCHAIN_REFERENCE_HEAD);
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
     NOT_IMPLEMENTED(ERROR);
     return false;
  }

  int64_t BlockChain::GetNumberOfBlocks() const{//TODO: refactor
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

  bool BlockChain::GetBlocks(json::Writer& writer) const{
    writer.StartArray();
    {
      BlockPtr current = GetHead();
      do{
        std::string hex = current->hash().HexString();
        writer.String(hex.data(), hex.length());
        current = GetBlock(current->GetPreviousHash());
      } while(current);
    }
    writer.EndArray();
    return true;
  }

  static JobQueue queue_(JobScheduler::kMaxNumberOfJobs);

  BlockChainPtr BlockChain::GetInstance(){
    //TODO: fix instantiation
    static std::shared_ptr<BlockChain> instance = std::make_shared<BlockChain>();
    return instance;
  }

  bool BlockChain::Initialize(const std::string& filename){
    if(!JobScheduler::RegisterQueue(GetCurrentThread(), &queue_)){
      LOG(ERROR) << "couldn't register BlockChain job queue.";
      return false;
    }

    leveldb::Status status;
    if(!(status = GetInstance()->InitializeIndex(filename)).ok()){
      LOG(ERROR) << "couldn't initialize the BlockChain in " << filename << ": " << status.ToString();
      return false;
    }
    return true;
  }
}