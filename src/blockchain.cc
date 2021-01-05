#include <mutex>
#include <sstream>
#include <leveldb/db.h>
#include <glog/logging.h>
#include <condition_variable>
#include "pool.h"
#include "keychain.h"
#include "blockchain.h"
#include "job/scheduler.h"
#include "job/processor.h"
#include "unclaimed_transaction.h"
#include "utils/timeline.h"

namespace Token{
  static inline std::string
  GetDataDirectory(){
    return FLAGS_path + "/data";
  }

  static inline std::string
  GetIndexFilename(){
    return GetDataDirectory() + "/index";
  }

  static inline std::string
  GetNewBlockFilename(const BlockPtr& blk){
    std::stringstream ss;
    ss << GetDataDirectory() + "/blk" << blk->GetHeight() << ".dat";
    return ss.str();
  }

  static inline std::string
  GetBlockHeightKey(int64_t height){
    std::stringstream ss;
    ss << "blk" << height;
    return ss.str();
  }

  static std::recursive_mutex mutex_;
  static std::condition_variable cond_;
  static BlockChain::State state_ = BlockChain::kUninitialized;
  static BlockChain::Status status_ = BlockChain::kOk;
  static leveldb::DB* index_ = nullptr;

#define LOCK_GUARD std::lock_guard<std::recursive_mutex> guard(mutex_)
#define LOCK std::unique_lock<std::recursive_mutex> lock(mutex_)
#define WAIT cond_.wait(lock)
#define SIGNAL_ONE cond_.notify_one()
#define SIGNAL_ALL cond_.notify_all()

  leveldb::DB* BlockChain::GetIndex(){
    return index_;
  }

  bool BlockChain::Initialize(){
    if(IsInitialized()){
      LOG(WARNING) << "cannot reinitialize the block chain!";
      return false;
    }

    if(!FileExists(GetDataDirectory())){
      if(!CreateDirectory(GetDataDirectory())){
        LOG(WARNING) << "couldn't create block chain index in directory: " << GetDataDirectory();
        return false;
      }
    }

    leveldb::Options options;
    options.create_if_missing = true;
    leveldb::Status status;
    if(!(status = leveldb::DB::Open(options, GetIndexFilename(), &index_)).ok()){
      LOG(WARNING) << "couldn't initialize the block chain index in " << GetIndexFilename() << ": " << status.ToString();
      return false;
    }

    LOG(INFO) << "initializing the block chain....";
    SetState(BlockChain::kInitializing);
    if(!HasReference(BLOCKCHAIN_REFERENCE_HEAD)){
      BlockPtr genesis = Block::Genesis();
      Hash hash = genesis->GetHash();

      // [Before - Work Stealing]
      //  - ProcessGenesisBlock Timeline (19s)
      // [After - Work Stealing]
      //  - ProcessGenesisBlock Timeline (4s)
      JobWorker* worker = JobScheduler::GetRandomWorker();
      ProcessBlockJob* job = new ProcessBlockJob(genesis);
      worker->Submit(job);
      worker->Wait(job);

      PutBlock(hash, genesis);
      PutReference(BLOCKCHAIN_REFERENCE_HEAD, hash);
      PutReference(BLOCKCHAIN_REFERENCE_GENESIS, hash);
    }

    LOG(INFO) << "block chain initialized!";
    BlockChain::SetState(BlockChain::kInitialized);
    return true;
  }

  BlockChain::State BlockChain::GetState(){
    LOCK_GUARD;
    return state_;
  }

  void BlockChain::SetState(State state){
    LOCK_GUARD;
    state_ = state;
    SIGNAL_ALL;
  }

  BlockChain::Status BlockChain::GetStatus(){
    LOCK_GUARD;
    return status_;
  }

  void BlockChain::SetStatus(Status status){
    LOCK_GUARD;
    status_ = status;
    SIGNAL_ALL;
  }

  BlockPtr BlockChain::GetGenesis(){
    LOCK_GUARD;
    return GetBlock(GetReference(BLOCKCHAIN_REFERENCE_GENESIS));
  }

  BlockPtr BlockChain::GetHead(){
    return GetBlock(GetReference(BLOCKCHAIN_REFERENCE_HEAD));
  }

  bool BlockChain::PutBlock(const Hash& hash, BlockPtr blk){
    if(HasBlock(hash)){
      LOG(WARNING) << "cannot overwrite existing block data for: " << hash;
      return false;
    }

    std::string filename = GetNewBlockFilename(blk);
    if(!blk->WriteToFile(filename)){
      LOG(WARNING) << "cannot write block data to file: " << filename;
      return false;
    }

    leveldb::Slice key((char*) hash.data(), Hash::GetSize());

    leveldb::WriteOptions opts;
    opts.sync = true;
    leveldb::Status status;
    if(!(status = GetIndex()->Put(opts, key, filename)).ok()){
      LOG(WARNING) << "cannot index object " << hash << ": " << status.ToString();
      return false;
    }

    LOG(INFO) << "indexed block: " << hash;
    return true;
  }

  BlockPtr BlockChain::GetBlock(const Hash& hash){
    leveldb::Slice key((char*) hash.data(), Hash::GetSize());
    std::string filename;

    leveldb::ReadOptions opts;
    leveldb::Status status;
    if(!(status = GetIndex()->Get(opts, key, &filename)).ok()){
      LOG(WARNING) << "cannot get " << hash << ": " << status.ToString();
      return BlockPtr(nullptr);
    }
    return Block::FromFile(filename);
  }

  BlockPtr BlockChain::GetBlock(int64_t height){
    //TODO: implement
    return BlockPtr(nullptr);
  }

  bool BlockChain::HasReference(const std::string& name){
    leveldb::ReadOptions options;
    std::string value;
    return GetIndex()->Get(options, name, &value).ok();
  }

  bool BlockChain::RemoveReference(const std::string& name){
    leveldb::WriteOptions options;
    std::string value;

    leveldb::Status status = GetIndex()->Delete(options, name);
    if(!status.ok()){
      LOG(WARNING) << "couldn't remove reference " << name << ": " << status.ToString();
      return false;
    }

    LOG(INFO) << "removed reference: " << name;
    return true;
  }

  bool BlockChain::PutReference(const std::string& name, const Hash& hash){
    leveldb::WriteOptions options;
    options.sync = true;

    leveldb::Status status = GetIndex()->Put(options, name, hash.HexString());
    if(!status.ok()){
      LOG(WARNING) << "couldn't set reference " << name << " to " << hash << ": " << status.ToString();
      return false;
    }

    LOG(INFO) << "set reference " << name << " := " << hash;
    return true;
  }

  Hash BlockChain::GetReference(const std::string& name){
    std::string value;

    leveldb::Status status;
    leveldb::ReadOptions options;
    if(!(status = GetIndex()->Get(options, name, &value)).ok()){
      LOG(WARNING) << "couldn't find reference " << name << ": " << status.ToString();
      return Hash();
    }

    return Hash::FromHexString(value);
  }

  bool BlockChain::HasBlock(const Hash& hash){
    leveldb::Slice key((char*) hash.data(), Hash::GetSize());

    leveldb::ReadOptions options;
    std::string filename;

    LOCK_GUARD;
    leveldb::Status status = GetIndex()->Get(options, key, &filename);
    if(!status.ok()){
      return false;
    }
    return true;
  }

  bool BlockChain::Append(const BlockPtr& block){
    LOCK_GUARD;
    BlockPtr head = GetHead();
    Hash hash = block->GetHash();
    Hash phash = block->GetPreviousHash();

    LOG(INFO) << "appending new block:";
    LOG(INFO) << "  - Parent Hash: " << phash;
    LOG(INFO) << "  - Hash: " << hash;
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

  bool BlockChain::VisitBlocks(BlockChainBlockVisitor* vis){
    Hash current = GetReference(BLOCKCHAIN_REFERENCE_HEAD);
    do{
      BlockPtr blk = GetBlock(current);
      if(!vis->Visit(blk))
        return false;
      current = blk->GetPreviousHash();
    } while(!current.IsNull());
    return true;
  }

  bool BlockChain::GetBlocks(HashList& hashes){
    Hash current = GetReference(BLOCKCHAIN_REFERENCE_HEAD);
    do{
      BlockPtr blk = GetBlock(current);
      hashes.insert(blk->GetHash());
      current = blk->GetPreviousHash();
    } while(!current.IsNull());
    return true;
  }

  bool BlockChain::VisitHeaders(BlockChainHeaderVisitor* vis){
    //TODO: Optimize BlockChain::VisitHeaders
    Hash current = GetReference(BLOCKCHAIN_REFERENCE_HEAD);
    do{
      BlockPtr blk = GetBlock(current);
      if(!vis->Visit(blk->GetHeader()))
        return false;
      current = blk->GetPreviousHash();
    } while(!current.IsNull());
    return true;
  }

  int64_t BlockChain::GetNumberOfBlocks(){
    int64_t count = 0;

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
}