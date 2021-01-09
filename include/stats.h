#ifndef TOKEN_STATS_H
#define TOKEN_STATS_H

#include "pool.h"
#include "blockchain.h"
#include "job/scheduler.h"
#include "utils/printer.h"

namespace Token{
  class StatsPrinter : Printer{
   public:
    StatsPrinter(const google::LogSeverity& severity, const long& flags=Printer::kFlagNone):
      Printer(severity, flags){}
    StatsPrinter(Printer* parent):
      Printer(parent){}
    ~StatsPrinter() = default;

    bool Print(const JobWorkerStats& stats) const{
      LOG_AT_LEVEL(GetSeverity()) << "Worker #" << stats.GetWorkerID();
      LOG_AT_LEVEL(GetSeverity()) << "\t- Number Of Jobs Ran: " << stats.GetNumberOfJobsRan();
      LOG_AT_LEVEL(GetSeverity()) << "\t- Number of Jobs Discarded: " << stats.GetNumberOfJobsDiscarded();
      LOG_AT_LEVEL(GetSeverity()) << "\t- Minimum Execution Time (Milliseconds): " << stats.GetMinimumTime();
      LOG_AT_LEVEL(GetSeverity()) << "\t- Mean Execution Time (Milliseconds): " << stats.GetMeanTime();
      LOG_AT_LEVEL(GetSeverity()) << "\t- Maximum Execution Time (Milliseconds): " << stats.GetMaximumTime();
      return true;
    }

    bool Print(const JobSchedulerStats& stats) const{
      LOG_AT_LEVEL(GetSeverity()) << "Job Scheduler:";
      LOG_AT_LEVEL(GetSeverity()) << "\t- Number of Jobs Ran: " << stats.GetNumberOfJobsRan();
      LOG_AT_LEVEL(GetSeverity()) << "\t- Number of Jobs Discarded: " << stats.GetNumberOfJobsDiscarded();
      if(IsDetailed()){
        for(int widx = 0; widx < FLAGS_num_workers; widx++){
          JobWorkerStats worker_stats = stats.GetWorkerStats(widx);
          if(!Print(worker_stats))
            return false;
        }
      }
      return true;
    }

    bool Print(const ObjectPoolStats& stats) const{
      LOG_AT_LEVEL(GetSeverity()) << "Object Pool:";
      LOG_AT_LEVEL(GetSeverity()) << "\t- Number of Objects: " << stats.GetNumberOfObjects();
      LOG_AT_LEVEL(GetSeverity()) << "\t- Number of Blocks: " << stats.GetNumberOfBlocks();
      LOG_AT_LEVEL(GetSeverity()) << "\t- Number of Transactions: " << stats.GetNumberOfTransactions();
      LOG_AT_LEVEL(GetSeverity()) << "\t- Number of Unclaimed Transactions: " << stats.GetNumberOfUnclaimedTransactions();
      return true;
    }

    bool Print(const BlockChainStats& stats) const{
      BlockHeader genesis = stats.GetGenesis();
      BlockHeader head = stats.GetHead();
      LOG_AT_LEVEL(GetSeverity()) << "Block Chain:";
      LOG_AT_LEVEL(GetSeverity()) << "\t- Total Number of Blocks: " << (head.GetHeight() + 1);
      LOG_AT_LEVEL(GetSeverity()) << "\t- Head: " << head.GetHash();
      LOG_AT_LEVEL(GetSeverity()) << "\t- Genesis: " << genesis.GetHash();
      return true;
    }

    static inline bool
    PrintAllStats(const google::LogSeverity& severity, const long& flags=Printer::kFlagNone){
      StatsPrinter printer(severity, flags);
      if(!printer.Print(BlockChain::GetStats())){
        LOG(WARNING) << "couldn't print the block chain stats.";
        return false;
      }

      if(!printer.Print(ObjectPool::GetStats())){
        LOG(WARNING) << "couldn't print the object pool stats.";
        return false;
      }

      if(!printer.Print(JobScheduler::GetStats())){
        LOG(WARNING) << "couldn't print the job scheduler stats.";
        return false;
      }
      return true;
    }

    static inline bool
    PrintAllStats(Printer* parent){
      StatsPrinter printer(parent);
      if(!printer.Print(BlockChain::GetStats())){
        LOG(WARNING) << "couldn't print the block chain stats.";
        return false;
      }

      if(!printer.Print(ObjectPool::GetStats())){
        LOG(WARNING) << "couldn't print the object pool stats.";
        return false;
      }

      if(!printer.Print(JobScheduler::GetStats())){
        LOG(WARNING) << "couldn't print the job scheduler stats.";
        return false;
      }
      return true;
    }

    template<class T>
    static inline bool
    PrintStats(const google::LogSeverity& severity, const long& flags=Printer::kFlagNone){
      StatsPrinter printer(severity, flags);
      return printer.Print(T::GetStats());
    }

    template<class T>
    static inline bool
    PrintStats(Printer* parent){
      StatsPrinter printer(parent);
      return printer.Print(T::GetStats());
    }

    static inline bool
    PrintJobSchedulerStats(const google::LogSeverity& severity, const long& flags=Printer::kFlagNone){
      return PrintStats<JobScheduler>(severity, flags);
    }

    static inline bool
    PrintJobSchedulerStats(Printer* parent){
      return PrintStats<JobScheduler>(parent);
    }

    static inline bool
    PrintBlockChainStats(const google::LogSeverity& severity, const long& flags=Printer::kFlagNone){
      return PrintStats<BlockChain>(severity, flags);
    }

    static inline bool
    PrintBlockChainStats(Printer* parent){
      return PrintStats<BlockChain>(parent);
    }

    static inline bool
    PrintObjectPoolStats(const google::LogSeverity& severity, const long& flags=Printer::kFlagNone){
      return PrintStats<ObjectPool>(severity, flags);
    }

    static inline bool
    PrintObjectPoolStats(Printer* parent){
      return PrintStats<ObjectPool>(parent);
    }
  };
}

#endif //TOKEN_STATS_H
