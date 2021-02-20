#include "block.h"
#include "block_file.h"

namespace token{
  static inline bool
  WriteBlockHeader(FILE* file, const BlockHeader& header){
    if(!WriteVersion(file, header.GetVersion())){
      LOG(ERROR) << "cannot write version to file.";
      return false;
    }

    if(!WriteTimestamp(file, header.GetTimestamp())){
      LOG(ERROR) << "cannot write timestamp to file.";
      return false;
    }

    if(!WriteLong(file, header.GetHeight())){
      LOG(ERROR) << "cannot write height to file.";
      return false;
    }

    if(!WriteHash(file, header.GetPreviousHash())){
      LOG(ERROR) << "cannot write previous hash to file.";
      return false;
    }

    if(!WriteHash(file, header.GetMerkleRoot())){
      LOG(ERROR) << "cannot write merkle root to file.";
      return false;
    }

    if(!WriteHash(file, header.GetHash())){
      LOG(ERROR) << "cannot write hash to file.";
      return false;
    }
    return true;
  }

  static inline bool
  WriteBlockTransactionData(FILE* file, const IndexedTransactionSet& txs){
    if(!WriteSet(file, txs)){
      LOG(ERROR) << "cannot write block transaction data to file.";
      return false;
    }
    return true;
  }

  bool WriteBlock(FILE* file, const BlockPtr& blk){
    if(file == NULL){
      LOG(WARNING) << "cannot write block to file, file is not open.";
      return false;
    }

    ObjectTag tag = blk->GetTag();
    if(!WriteTag(file, tag)){
      LOG(ERROR) << "cannot write block tag to file.";
      return false;
    }

    BlockHeader header = blk->GetHeader();
    if(!WriteBlockHeader(file, header)){
      LOG(ERROR) << "cannot write block header to file.";
      return false;
    }

    if(!WriteBlockTransactionData(file, blk->transactions())){
      LOG(ERROR) << "cannot write block transaction data to file.";
      return false;
    }
    return true;
  }

  static inline bool
  ReadBlockHeader(FILE* file, BlockHeader* header){
    BufferPtr buffer = Buffer::NewInstance(BlockHeader::GetSize());
    if(!ReadBytes(file, buffer, BlockHeader::GetSize()))
      return false;
    (*header) = BlockHeader(buffer);
    return true;
  }

  BlockPtr ReadBlock(FILE* file){
    if(file == NULL){
      LOG(ERROR) << "cannot read block.";
      return BlockPtr(nullptr);
    }

    ObjectTag tag = ReadTag(file);
    if(!tag.IsValid()){
      LOG(ERROR) << "file has invalid tag: " << tag;
      return BlockPtr(nullptr);
    }

    if(!tag.IsBlockType()){
      LOG(ERROR) << "file has invalid tag: " << tag;
      return BlockPtr(nullptr);
    }

    BlockHeader header;
    if(!ReadBlockHeader(file, &header)){
      LOG(ERROR) << "cannot read block header.";
      return BlockPtr(nullptr);
    }

    IndexedTransactionSet txs;
    if(!ReadSet(file, Type::kTransaction, txs)){
      LOG(ERROR) << "cannot read transaction data.";
      return BlockPtr(nullptr);
    }

    return Block::NewInstance(
      header.GetHeight(),
      header.GetVersion(),
      header.GetPreviousHash(),
      txs,
      header.GetTimestamp()
    );
  }
}