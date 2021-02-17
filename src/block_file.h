#ifndef TOKEN_BLOCK_WRITER_H
#define TOKEN_BLOCK_WRITER_H

#include <cstdio>
#include "block_header.h"
#include "utils/file_reader.h"
#include "utils/file_writer.h"

namespace token{
  class BlockFileWriter : TaggedFileWriter{
    //TODO:
    // - optimize, in-efficient copying during writing process
   private:
    BlockPtr block_;

    bool WriteHeader(const BlockHeader& header) const{
      BufferPtr buffer = header.ToBuffer();
      if(!WriteBytes((uint8_t*)buffer->data(), buffer->GetWrittenBytes())){
        LOG(ERROR) << "cannot write block header " << header << " to " << GetFilename();
        return false;
      }
      return true;
    }

    bool WriteBloomFilter(const BloomFilter& bloom) const{
      //TODO: implement WriteBloomFilter(const BloomFilter&)
      return true;
    }

    bool WriteTransactions(const IndexedTransactionSet& transactions) const{
      return WriteSet(transactions);
    }
   public:
    BlockFileWriter(const std::string& filename, const BlockPtr& blk):
      TaggedFileWriter(filename, blk->GetTag()),
      block_(blk){}
    ~BlockFileWriter() = default;

    bool Write() const{
      BlockHeader header(block_);
      return WriteTag()
          && WriteHeader(header)
          && WriteBloomFilter(block_->GetBloomFilter())
          && WriteTransactions(block_->transactions());
    }
  };

  class BlockFileReader : TaggedFileReader{
   public:
    BlockFileReader(const std::string& filename):
      TaggedFileReader(filename){}
    ~BlockFileReader() = default;

    bool HasValidTag() const{
      if(!Seek(0)){ //Make a constant
        LOG(WARNING) << "cannot seek to the block file tag position of file " << GetFilename();
        return false;
      }

      ObjectTag tag = ReadTag();
      if(!tag.IsValid()){
        LOG(WARNING) << "block file " << GetFilename() << " has an invalid tag: " << tag;
        return false;
      }

      if(!tag.IsBlockType()){
        LOG(WARNING) << "block file " << GetFilename() << " tag " << tag << " is not a block.";
        return false;
      }
      return true;
    }

    bool ReadHeaderData(BlockHeader* result) const{
      if(!Seek(sizeof(RawObjectTag))){ // make constant
        LOG(WARNING) << "cannot seek to the block file header position of file " << GetFilename();
        return false;
      }

      BufferPtr buffer = Buffer::NewInstance(BlockHeader::GetSize());
      if(!ReadBytes((uint8_t*)buffer->data(), BlockHeader::GetSize()))
        return false;
      (*result) = BlockHeader(buffer);
      return true;
    }

    bool ReadTransactionData(IndexedTransactionSet& transactions) const{
      if(!Seek(sizeof(RawObjectTag)+BlockHeader::GetSize())){ // make constant
        LOG(WARNING) << "cannot seek to the block file data position of file " << GetFilename();
        return false;
      }

      int64_t ntxs = ReadLong();
      for(int64_t idx = 0; idx < ntxs; idx++){
        ObjectTag tag = ReadTag();
        if(!tag.IsValid()){
          LOG(WARNING) << "block file " << GetFilename() << " transaction #" << idx << " has an invalid tag: " << tag;
          return false;
        }

        if(!tag.IsTransactionType()){
          LOG(WARNING) << "block file " << GetFilename() << " transaction #" << idx << " " << tag << " tag is not a transaction";
          return false;
        }

        BufferPtr buffer = Buffer::NewInstance(tag.GetSize());
        if(!ReadBytes((uint8_t*)buffer->data(), tag.GetSize())){
          LOG(WARNING) << "block file " << GetFilename() << " transaction #" << idx << " cannot read transaction data.";
          return false;
        }

        IndexedTransactionPtr tx = IndexedTransaction::FromBytes(buffer);
        if(!transactions.insert(tx).second){
          LOG(WARNING) << "block file " << GetFilename() << " transaction #" << idx << " cannot add transaction " << tx->GetHash() << " to transaction set.";
          return false;
        }
      }
      return true;
    }

    BlockPtr ReadBlockData() const{
      BlockHeader header;
      if(!ReadHeaderData(&header)){
        LOG(WARNING) << "cannot read block header.";
        return BlockPtr(nullptr);
      }

      IndexedTransactionSet data;
      if(!ReadTransactionData(data)){
        LOG(WARNING) << "cannot read block transaction data.";
        return BlockPtr(nullptr);
      }

      BlockPtr blk = Block::NewInstance(header.GetHeight(), header.GetVersion(), header.GetPreviousHash(), data, header.GetTimestamp());
      if(blk->GetHash() != header.GetHash()){
        LOG(WARNING) << "block file " << GetFilename() << " has invalid transaction data, expected: " << header;
        return BlockPtr(nullptr);
      }
      return blk;
    }
  };
}

#endif//TOKEN_BLOCK_WRITER_H