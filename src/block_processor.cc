#include "block_processor.h"

#include "transaction_verifier.h"

namespace Token{
    class ProcessOutputsJob : public Job{
    public:
        static const int64_t kMaxNumberOfOutputs = Block::kMaxTransactionsForBlock / JobPool::kMaxNumberOfWorkers / 4;
    private:
        int64_t worker_;
        int64_t offset_;
        OutputList outputs_;

        inline UnclaimedTransactionPtr
        CreateUnclaimedTransaction(const Output& output){
            TransactionPtr tx = GetTransaction();
            int64_t index = GetNextOutputIndex();
            return UnclaimedTransactionPtr(new UnclaimedTransaction(tx->GetHash(), index, output.GetUser(), output.GetProduct()));
        }

        bool Process(const Output& output){
            UnclaimedTransactionPtr utxo = CreateUnclaimedTransaction(output);
            return UnclaimedTransactionPool::PutUnclaimedTransaction(utxo->GetHash(), utxo);
        }

        int64_t GetNextOutputIndex(){
            return offset_++;
        }
    protected:
        JobResult DoWork(){
            LOG(INFO) << "processing " << outputs_.size() << " outputs....";
            for(auto& it : outputs_){
                if(!Process(it)){
                    std::stringstream ss;
                    ss << "[" << GetName() << "-#" << GetWorkerID() << "] ";
                    ss << "Cannot process output #" << GetCurrentOutputIndex() << " (" << it.ToString() << ")";
                    return Failed(ss.str());
                }
            }
            return Success("Finished.");
        }
    public:
        ProcessOutputsJob(ProcessTransactionJob* parent, int64_t worker_id, const OutputList& outputs):
            Job(parent, "ProcessOutputs"),
            worker_(worker_id),
            offset_(worker_id * ProcessOutputsJob::kMaxNumberOfOutputs),
            outputs_(outputs){}
        ~ProcessOutputsJob() = default;

        int64_t GetWorkerID() const{
            return worker_;
        }

        TransactionPtr GetTransaction() const{
            ProcessTransactionJob* parent = (ProcessTransactionJob*)GetParent();
            return parent->GetTransaction();
        }

        int64_t GetCurrentOutputIndex() const{
            return offset_;
        }
    };

    ProcessTransactionJob::ProcessTransactionJob(ProcessBlockJob* parent, const TransactionPtr& tx):
        Job(parent, "ProcessTransaction"),
        transaction_(tx){}

    JobResult ProcessTransactionJob::DoWork(){
        TransactionPtr tx = GetTransaction();

        int64_t nworker = 0;
        OutputList& outputs = tx->outputs();
        auto start = outputs.begin();
        auto end = outputs.end();
        while(start != end){
            auto next = std::distance(start, end) > ProcessOutputsJob::kMaxNumberOfOutputs
                        ? start + ProcessOutputsJob::kMaxNumberOfOutputs
                        : end;

            OutputList chunk(start, next);
            ProcessOutputsJob* job = new ProcessOutputsJob(this, nworker++, chunk);
            if(!JobPool::Schedule(job))
                return Failed("Cannot schedule ProcessOutputsJob()");

            start = next;
        }
        return Success("Finished.");
    }
}