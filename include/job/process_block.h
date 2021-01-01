#ifndef TOKEN_PROCESS_BLOCK_H
#define TOKEN_PROCESS_BLOCK_H

#include "pool.h"
#include "block.h"
#include "job/job.h"

namespace Token{
  //TODO: fixme
  typedef std::map<std::string, HashList> UserHashLists;

  class ProcessBlockJob : public Job, BlockVisitor{
    friend class ProcessTransactionJob;
   protected:
    BlockPtr block_;
    std::mutex mutex_; //TODO: remove mutex
    leveldb::WriteBatch* batch_;
    UserHashLists* hash_lists_;
    bool clean_; //TODO: remove clean flag?

    JobResult DoWork();
   public:
    ProcessBlockJob(const BlockPtr& blk, bool clean = false):
      Job(nullptr, "ProcessBlock"),
      block_(blk),
      mutex_(),
      batch_(new leveldb::WriteBatch()),
      hash_lists_(new UserHashLists()),
      clean_(clean){}
    ~ProcessBlockJob(){
      if(batch_)
        delete batch_;
    }

    leveldb::WriteBatch* GetBatch() const{
      return batch_;
    }

    BlockPtr GetBlock() const{
      return block_;
    }

    bool ShouldClean() const{
      return clean_;
    }

    bool Visit(const TransactionPtr& tx);

    void Append(leveldb::WriteBatch* batch, const UserHashLists& hashes){
      std::lock_guard<std::mutex> guard(mutex_);
      batch_->Append((*batch));

      for(auto& it : hashes){
        LOG(INFO) << "appending " << it.second.size() << " hashes for " << it.first;
        auto pos = hash_lists_->find(it.first);
        if(pos == hash_lists_->end()){
          LOG(INFO) << "creating new hash list";
          hash_lists_->insert({it.first, it.second});
          continue;
        }

        const HashList& src = it.second;
        HashList& dst = pos->second;
        dst.insert(src.begin(), src.end());
        hash_lists_->insert({it.first, dst});
      }
    }

    leveldb::Status Write(){
      std::lock_guard<std::mutex> guard(mutex_);
      if(!batch_)
        return leveldb::Status::InvalidArgument("batch_ is NULL");
      return ObjectPool::Write(batch_);
    }
  };
}

#endif //TOKEN_PROCESS_BLOCK_H