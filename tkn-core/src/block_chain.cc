#include "block_chain.h"

#include "dbutils.h"
#include "filesystem.h"

namespace token{
  BlockChain::BlockChain(const std::string& path):
    filename_(path + "/chain"),
    state_(State::kUninitialized),
    index_(nullptr){
    Initialize();
  }

  BlockChain::~BlockChain(){
    delete index_;
  }

  void BlockChain::Initialize(){
    if(!IsUninitialized()){
      LOG(ERROR) << "cannot re-initialize the BlockChain.";
      return;
    }

    DLOG(INFO) << "loading BlockChain from " << GetFilename() << "....";
    if(!FileExists(GetFilename())){
      if(!CreateDirectory(GetFilename())){
        LOG(FATAL) << "cannot create the BlockChain directory: " << GetFilename();
        return;
      }
    }

    leveldb::Options options;
    options.create_if_missing = true;
    options.comparator = new BlockChain::Comparator();

    leveldb::Status status;
    if(!(status = leveldb::DB::Open(options, GetIndexFilename(), &index_)).ok()){
      LOG(FATAL) << "couldn't initialize the BlockChain index: " << status;
      return;
    }

    //TODO: initialize the BlockChain more?
  }

  static inline void
  WriteBlock(const BlockPtr& blk, const std::string& filename){
    platform::fs::File* file;//TODO: make portable
    if(!(file = fopen(filename.data(), "wb"))){
      LOG(FATAL) << "cannot open block file " << filename << ": " << strerror(errno);
      return;
    }

//TODO:
//    Block::Encoder encoder(blk.get(), codec::kDefaultEncoderFlags); //TODO: change encoder flags
//    BufferPtr data = internal::NewBufferFor(encoder);
//    if(!encoder.Encode(data)){
//      LOG(FATAL) << "cannot encode write block " << blk->hash() << " to buffer.";
//      return;
//    }
//    if(!data->WriteTo(file)){
//      LOG(FATAL) << "cannot write buffer to file: " << filename;
//      return;
//    }

    platform::fs::Flush(file);
    platform::fs::Close(file);
  }

  bool BlockChain::PutBlock(const Hash& hash, const BlockPtr& blk) const{
    ObjectKey key(Type::kBlock, hash);
    std::string filename = GetBlockFilename(blk);

    leveldb::WriteOptions opts;
    opts.sync = true;

    leveldb::Status status;
    if(!(status = GetIndex()->Put(opts, (const leveldb::Slice&)key, filename)).ok()){
      LOG(WARNING) << "cannot index object " << hash << ": " << status.ToString();
      return false;
    }

    WriteBlock(blk, filename);

    DLOG(INFO) << "indexed block: " << hash;
    return true;
  }

  static inline BlockPtr
  ReadBlockFile(const std::string& filename, const Hash& hash){
    auto filesize = GetFilesize(filename);//TODO: cleanup code
    BufferPtr buffer = internal::NewBufferFromFile(filename, filesize);
//TODO: BlockPtr block = Block::Decode(buffer, codec::ExpectTypeHint::Encode(true)|codec::ExpectVersionHint::Encode(true));
    BlockPtr block = Block::Genesis();
    LOG_IF(FATAL, block->hash() != hash) << "expected block hash of " << hash << ", but parsed block was: " << block->ToString();
    return block;
  }

  BlockPtr BlockChain::GetBlock(const Hash& hash) const{
    ObjectKey key(Type::kBlock, hash);

    std::string filename;
    leveldb::Status status;
    if(!(status = GetIndex()->Get(leveldb::ReadOptions(), (const leveldb::Slice&)key, &filename)).ok()){
      if(status.IsNotFound()){
        LOG(WARNING) << "couldn't find block: " << hash;
        return nullptr;
      }

      LOG(FATAL) << "cannot get block " << hash << ": " << status;
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
      LOG(ERROR) << "couldn't remove block " << hash << ": " << status.ToString();
      return false;
    }

    DLOG(INFO) << "removed block: " << hash;
    return true;
  }

  bool BlockChain::HasBlock(const Hash& hash) const{
    ObjectKey key(Type::kBlock, hash);

    std::string filename;
    leveldb::Status status;
    if(!(status = GetIndex()->Get(leveldb::ReadOptions(), (const leveldb::Slice&)key, &filename)).ok()){
      if(status.IsNotFound())
        return false;

      LOG(ERROR) << "couldn't get block " << hash << ": " << status.ToString();
      return false;
    }
    return true;
  }

  //TODO: cleanup logging
  bool BlockChain::Append(const BlockPtr& block){
    BlockPtr head = GetHead();
    Hash hash = block->hash();
    Hash phash = block->previous_hash();

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
      //TODO: update <HEAD> reference
    }
    return true;
  }

  bool BlockChain::VisitBlocks(BlockChainBlockVisitor* vis) const{
    Hash current; //TODO: get <HEAD>
    do{
      BlockPtr blk = GetBlock(current);
      if(!vis->Visit(blk))
        return false;
      current = blk->previous_hash();
    } while(!current.IsNull());
    return true;
  }

  static inline bool
  IsValidBlock(const std::string& filename){
    NOT_IMPLEMENTED(ERROR);
    return false;
  }

  int64_t BlockChain::GetNumberOfBlocks() const{//TODO: refactor
    std::string home = GetDataDirectory();

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
        current = GetBlock(current->previous_hash());
      } while(current);
    }
    writer.EndArray();
    return true;
  }
}