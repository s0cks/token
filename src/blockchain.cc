#include <sstream>
#include <leveldb/db.h>
#include <glog/logging.h>
#include <atomic/relaxed_atomic.h>

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

  static RelaxedAtomic<BlockChain::State> state_ = { BlockChain::kUninitialized };
  static JobQueue queue_(JobScheduler::kMaxNumberOfJobs);
  static leveldb::DB* index_ = nullptr;

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
    options.comparator = new ObjectKey::Comparator();

    leveldb::Status status;
    if(!(status = leveldb::DB::Open(options, GetIndexFilename(), &index_)).ok()){
      LOG(WARNING) << "couldn't initialize the block chain index in " << GetIndexFilename() << ": "
                   << status.ToString();
      return false;
    }

    LOG(INFO) << "initializing the block chain....";
    SetState(BlockChain::kInitializing);
    if(!JobScheduler::RegisterQueue(pthread_self(), &queue_)){
      LOG(WARNING) << "couldn't register the block chain work queue.";
      return false;
    }

    if(!HasReference(BLOCKCHAIN_REFERENCE_HEAD)){
      BlockPtr genesis = Block::Genesis();
      Hash hash = genesis->GetHash();

      // [Before - Work Stealing]
      //  - ProcessGenesisBlock Timeline (19s)
      // [After - Work Stealing]
      //  - ProcessGenesisBlock Timeline (4s)
      JobQueue* queue = JobScheduler::GetThreadQueue();
      ProcessBlockJob* job = new ProcessBlockJob(genesis);
      queue->Push(job);
      while(!job->IsFinished()); //spin

      if(!job->CommitAllChanges()){
        LOG(ERROR) << "couldn't commit changes.";
        SetState(BlockChain::kUninitialized);
        return false;
      }

      PutBlock(hash, genesis);
      PutReference(BLOCKCHAIN_REFERENCE_HEAD, hash);
      PutReference(BLOCKCHAIN_REFERENCE_GENESIS, hash);
    }

    LOG(INFO) << "block chain initialized!";
    BlockChain::SetState(BlockChain::kInitialized);
    return true;
  }

  BlockChain::State BlockChain::GetState(){
    return state_;
  }

  void BlockChain::SetState(const State& state){
    state_ = state;
  }

  BlockPtr BlockChain::GetGenesis(){
    return GetBlock(GetReference(BLOCKCHAIN_REFERENCE_GENESIS));
  }

  BlockPtr BlockChain::GetHead(){
    return GetBlock(GetReference(BLOCKCHAIN_REFERENCE_HEAD));
  }

  static inline bool
  WriteBlockData(const std::string& filename, const BlockPtr& blk){
    BinaryObjectFileWriter writer(filename);
    return writer.WriteObject(blk);
  }

  bool BlockChain::PutBlock(const Hash& hash, BlockPtr blk){
    ObjectKey okey(Type::kBlockType, hash);
    std::string filename = GetNewBlockFilename(blk);
    if(!WriteBlockData(filename, blk)){
      LOG(WARNING) << "cannot write block data to file: " << filename;
      return false;
    }

    leveldb::Slice key(okey.data(), okey.size());
    leveldb::Slice value(filename.data(), filename.size());

    leveldb::WriteOptions opts;
    opts.sync = true;
    leveldb::Status status;
    if(!(status = GetIndex()->Put(opts, key, value)).ok()){
      LOG(WARNING) << "cannot index object " << hash << ": " << status.ToString();
      return false;
    }

    LOG(INFO) << "indexed block: " << hash;
    return true;
  }

  BlockPtr BlockChain::GetBlock(const Hash& hash){
    ObjectKey okey(Type::kBlockType, hash);

    std::string filename;
    leveldb::Status status;
    leveldb::Slice key(okey.data(), okey.size());
    if(!(status = GetIndex()->Get(leveldb::ReadOptions(), key, &filename)).ok()){
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
    ObjectKey okey(Type::kReferenceType, name);

    std::string value;
    leveldb::Slice key(okey.data(), okey.size());
    return GetIndex()->Get(leveldb::ReadOptions(), key, &value).ok();
  }

  bool BlockChain::RemoveReference(const std::string& name){
    leveldb::WriteOptions options;
    options.sync = true;

    ObjectKey okey(Type::kReferenceType, name);

    std::string value;
    leveldb::Slice key(okey.data(), okey.size());
    leveldb::Status status;
    if(!(status = GetIndex()->Delete(options, key)).ok()){
      LOG(WARNING) << "couldn't remove reference " << name << ": " << status.ToString();
      return false;
    }

    LOG(INFO) << "removed reference: " << name;
    return true;
  }

  bool BlockChain::PutReference(const std::string& name, const Hash& hash){
    leveldb::WriteOptions options;
    options.sync = true;

    ObjectKey okey(Type::kReferenceType, name);

    leveldb::Status status;
    leveldb::Slice key(okey.data(), okey.size());
    if(!(status = GetIndex()->Put(options, key, hash.HexString())).ok()){
      LOG(WARNING) << "couldn't set reference " << name << " to " << hash << ": " << status.ToString();
      return false;
    }

    LOG(INFO) << "set reference " << name << " := " << hash;
    return true;
  }

  Hash BlockChain::GetReference(const std::string& name){
    ObjectKey okey(Type::kReferenceType, name);

    std::string value;
    leveldb::Status status;
    leveldb::Slice key(okey.data(), okey.size());
    if(!(status = GetIndex()->Get(leveldb::ReadOptions(), key, &value)).ok()){
      LOG(WARNING) << "couldn't find reference " << name << ": " << status.ToString();
      return Hash();
    }

    return Hash::FromHexString(value);
  }

  bool BlockChain::HasBlock(const Hash& hash){
    ObjectKey okey(Type::kBlockType, hash);

    std::string filename;
    leveldb::Slice key(okey.data(), okey.size());
    leveldb::Status status;
    if(!(status = GetIndex()->Get(leveldb::ReadOptions(), key, &filename)).ok()){
      LOG(WARNING) << "cannot find block " << hash << ": " << status.ToString();
      return false;
    }
    return true;
  }

  bool BlockChain::Append(const BlockPtr& block){
    BlockPtr head = GetHead();
    Hash hash = block->GetHash();
    Hash phash = block->GetPreviousHash();

    LOG(INFO) << "appending new block:";
    PrettyPrinter::PrettyPrint(block);
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

  bool BlockChain::VisitHeaders(BlockChainHeaderVisitor* vis){
    //TODO: Optimize BlockChain::VisitHeaders
    Hash current = GetReference(BLOCKCHAIN_REFERENCE_HEAD);
    do{
      BlockPtr blk = GetBlock(current);
      if(!vis->Visit(blk->GetHeader())){
        return false;
      }
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

  bool BlockChain::GetBlocks(Json::Writer& writer){
    writer.StartArray();
    {
      Hash current = GetReference(BLOCKCHAIN_REFERENCE_HEAD);
      do{
        BlockPtr blk = GetBlock(current);
        Json::Append(writer, current);
        current = blk->GetPreviousHash();
      } while(!current.IsNull());
      return true;
    }
    writer.EndArray();
    return true;
  }

#ifdef TOKEN_DEBUG
  bool BlockChain::GetStats(Json::Writer& writer){
    writer.StartObject();
    {
      Json::SetField(writer, "head", GetReference(BLOCKCHAIN_REFERENCE_HEAD));
      Json::SetField(writer, "genesis", GetReference(BLOCKCHAIN_REFERENCE_GENESIS));
    }
    writer.EndObject();
    return true;
  }
#endif//TOKEN_DEBUG
}