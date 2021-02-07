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

namespace token{
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
    options.comparator = new BlockChain::Comparator();

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

  class BlockWriter{
   private:
    std::string filename_;
    BlockPtr block_;
   public:
    BlockWriter(const std::string& filename, const BlockPtr& blk):
      filename_(filename),
      block_(blk){}
    ~BlockWriter() = default;

    const char* GetFilename() const{
      return filename_.data();
    }

    BlockPtr& GetBlock(){
      return block_;
    }

    Hash GetHash() const{
      return block_->GetHash();
    }

    bool Write() const{
      FILE* file = NULL;
      if(!(file = fopen(GetFilename(), "wb"))){
        LOG(WARNING) << "cannot write block " << GetHash() << " to file: " << GetFilename();
        return false;
      }

      int64_t size = sizeof(RawObjectTag)
                   + BlockHeader::kSize
                   + block_->GetTransactionDataBufferSize();
      ObjectTag tag(Type::kBlock, size);
      BlockHeader header(block_);
      TransactionSet& transactions = block_->transactions();

      BufferPtr buffer = Buffer::NewInstance(size);
      if(!buffer->PutObjectTag(tag)){
        LOG(WARNING) << "cannot write object tag " << tag << " to buffer of size: " << size;
        return false;
      }

      if(!header.Write(buffer)){
        LOG(WARNING) << "cannot write block header " << header << " to buffer of size: " << size;
        return false;
      }

      if(!buffer->PutSet(transactions)){
        LOG(WARNING) << "cannot write block transaction data to buffer of size: " << size;
        return false;
      }

      int err;
      if((err = fwrite(buffer->data(), sizeof(uint8_t), buffer->GetWrittenBytes(), file)) != buffer->GetWrittenBytes()){
        LOG(WARNING) << "cannot write buffer of size " << size << " to file " << GetFilename() << ": " << strerror(err);
        return false;
      }
      return true;
    }
  };

  bool BlockChain::PutBlock(const Hash& hash, BlockPtr blk){
    BlockKey key(blk);
    std::string filename = GetNewBlockFilename(blk);

    leveldb::WriteOptions opts;
    opts.sync = true;

    leveldb::Status status;
    if(!(status = GetIndex()->Put(opts, key, filename)).ok()){
      LOG(WARNING) << "cannot index object " << hash << ": " << status.ToString();
      return false;
    }

    BlockWriter writer(filename, blk);
    if(!writer.Write()){
      LOG(WARNING) << "cannot write block " << hash << " to file: " << filename;
      return false; //TODO: remove key
    }

    LOG(INFO) << "indexed block: " << hash;
    return true;
  }

  BlockPtr BlockChain::GetBlock(const Hash& hash){
    BlockKey key(1, 0, hash);

    std::string filename;
    leveldb::Status status;
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
    ReferenceKey key(name);
    std::string value;
    return GetIndex()->Get(leveldb::ReadOptions(), key, &value).ok();
  }

  bool BlockChain::RemoveReference(const std::string& name){
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

  bool BlockChain::PutReference(const std::string& name, const Hash& hash){
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

  Hash BlockChain::GetReference(const std::string& name){
    ReferenceKey key(name);
    std::string value;

    leveldb::Status status;
    if(!(status = GetIndex()->Get(leveldb::ReadOptions(), key, &value)).ok()){
      LOG(WARNING) << "couldn't find reference " << name << ": " << status.ToString();
      return Hash();
    }

    return Hash::FromHexString(value);
  }

  bool BlockChain::HasBlock(const Hash& hash){
    BlockKey key(0, 0, hash);
    std::string filename;

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

  static inline bool
  IsValidBlock(const std::string& filename){
    FILE* file = NULL;
    if((file = fopen(filename.data(), "rb")) == NULL){
      LOG(WARNING) << "cannot open block file " << filename;
      return false;
    }

    int err;

    RawObjectTag raw_tag = 0;
    if((err = fread(&raw_tag, sizeof(RawObjectTag), 1, file)) != 1){
      LOG(WARNING) << "cannot read object tag from file " << filename << ": " << strerror(err);
      return false;
    }

    ObjectTag tag(raw_tag);
    if(!tag.IsValid()){
      LOG(WARNING) << "object tag " << tag << " is not valid.";
    } else if(!tag.IsBlockType()){
      LOG(WARNING) << "object tag " << tag << " is not a block.";
      return false;
    }

    LOG(INFO) << "read object tag: " << tag;

    uint8_t buff[BlockHeader::kSize];
    if((err = fread(buff, BlockHeader::kSize, 1, file)) != 1){
      LOG(WARNING) << "cannot read block header from file " << filename << ": " << strerror(err);
      return false;
    }

    BlockHeader header(Buffer::From(buff, BlockHeader::kSize));
    LOG(INFO) << "read block header: " << header;
    return true;
  }

  int64_t BlockChain::GetNumberOfBlocks(){
    int64_t count = 0;

    DIR* dir;
    struct dirent* ent;
    if((dir = opendir(GetDataDirectory().c_str())) != NULL){
      while((ent = readdir(dir)) != NULL){
        std::string filename(ent->d_name);
        std::string abs_path = GetDataDirectory() + '/' + filename;
        if(EndsWith(filename, ".dat") && IsValidBlock(abs_path)){
          count++;
        }
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