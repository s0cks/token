#ifndef TKN_BLOCK_CHAIN_H
#define TKN_BLOCK_CHAIN_H

#include <iostream>

#include <leveldb/db.h>
#include <leveldb/comparator.h>

#include "key.h"
#include "block.h"
#include "atomic/relaxed_atomic.h"

namespace token{
#define FOR_EACH_BLOCKCHAIN_STATE(V) \
    V(Uninitialized)                 \
    V(Initializing)                  \
    V(Initialized)                   \
    V(Synchronizing)

  class BlockChainBlockVisitor;
  class BlockChain{
  public:
    enum State{
#define DEFINE_STATE(Name) k##Name,
      FOR_EACH_BLOCKCHAIN_STATE(DEFINE_STATE)
#undef DEFINE_STATE
    };

    friend std::ostream& operator<<(std::ostream& stream, const State& state){
      switch(state){
#define DEFINE_TOSTRING(Name) \
        case State::k##Name:  \
          return stream << #Name;
        FOR_EACH_BLOCKCHAIN_STATE(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
        default:
          return stream << "Unknown";
      }
    }

    class Comparator : public leveldb::Comparator{
    public:
      Comparator() = default;
      ~Comparator() override = default;

      int Compare(const leveldb::Slice& a, const leveldb::Slice& b) const override{
        ObjectKey k1(a), k2(b);
        return ObjectKey::Compare(k1, k2);
      }

      const char* Name() const override{
        return "BlockComparator";
      }

      void FindShortestSeparator(std::string* str, const leveldb::Slice& slice) const override{}
      void FindShortSuccessor(std::string* str) const override{}
    };
  protected:
    atomic::RelaxedAtomic<State> state_;
    leveldb::DB* index_;
    std::string filename_;

    inline std::string
    GetDataDirectory() const{
      return GetFilename() + "/data";
    }

    inline std::string
    GetIndexFilename() const{
      return GetFilename() + "/index";
    }

    inline std::string
    GetBlockFilename(const BlockPtr& blk) const{
      std::stringstream ss;
      ss << GetDataDirectory() << "/blk" << blk->height() << ".dat";
      return ss.str();
    }

    inline leveldb::DB*
    GetIndex() const{
      return index_;
    }

    /**
     * TODO
     * @param state
     */
    void SetState(const State& state){
      state_ = state;
    }

    bool PutBlock(const Hash& hash, const BlockPtr& blk) const;
    bool RemoveBlock(const Hash& hash, const BlockPtr& blk) const;
    bool Append(const BlockPtr& blk);

    void Initialize();
  public:
    explicit BlockChain(const std::string& path);
    virtual ~BlockChain();

    /**
     * TODO
     * @return
     */
    BlockChain::State GetState() const{
      return (BlockChain::State)state_;
    }

    /**
     * TODO
     * @param hash
     * @return
     */
    virtual bool HasBlock(const Hash& hash) const;

    /**
     * TODO
     */
    bool VisitBlocks(BlockChainBlockVisitor* vis) const;

    /**
     * TODO
     * @param hash
     * @return
     */
    virtual BlockPtr GetBlock(const Hash& hash) const;

    /**
     * TODO
     * @param height
     * @return
     */
    BlockPtr GetBlock(const int64_t& height) const;

    /**
     * TODO
     * @return
     */
    int64_t GetNumberOfBlocks() const;

    /**
     * TODO
     * @return
     */
    int64_t GetNumberOfReferences() const;

    //TODO: guard
    bool GetBlocks(json::Writer& writer) const;

    /**
     * TODO
     * @return
     */
    inline bool
    HasBlocks() const{
      return GetNumberOfBlocks() > 0;
    }

    virtual inline Hash
    GetHeadHash() const{
      //TODO: implement:
      //return config::GetHash(BLOCKCHAIN_REFERENCE_HEAD);
      return Hash();
    }

    virtual inline BlockPtr
    GetHead() const{
      return GetBlock(GetHeadHash());
    }

    virtual inline Hash
    GetGenesisHash() const{
      //TODO: implement:
      //return config::GetHash(BLOCKCHAIN_REFERENCE_GENESIS);
      return Hash();
    }

    virtual inline BlockPtr
    GetGenesis() const{
      return GetBlock(GetGenesisHash());
    }

    std::string GetFilename() const{
      return filename_;
    }

#define DEFINE_STATE_CHECK(Name) \
    inline bool Is##Name() const{ return GetState() == State::k##Name; }
    FOR_EACH_BLOCKCHAIN_STATE(DEFINE_STATE_CHECK)
#undef DEFINE_STATE_CHECK
  };

  class BlockChainBlockVisitor{
  protected:
    BlockChainBlockVisitor() = default;
  public:
    virtual ~BlockChainBlockVisitor() = default;
    virtual bool Visit(const BlockPtr& blk) = 0;
  };
}

#endif//TKN_BLOCK_CHAIN_H