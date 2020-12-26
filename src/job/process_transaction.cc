#include "job/scheduler.h"
#include "job/process_transaction.h"

namespace Token{
    class ProcessInputListJob : public Job{
    public:
        static const int64_t kMaxNumberOfInputs = 128;
    private:
        InputList inputs_;
    protected:
        JobResult DoWork(){
            for(auto& it : inputs_){
                UnclaimedTransactionPtr utxo = it.GetUnclaimedTransaction();
                Hash hash = utxo->GetHash();
                if(!UnclaimedTransactionPool::RemoveUnclaimedTransaction(hash)){
                    std::stringstream ss;
                    ss << "Couldn't remove unclaimed transaction: " << hash;
                    return Failed(ss.str());
                }
            }
            return Success("done.");
        }
    public:
        ProcessInputListJob(ProcessTransactionInputsJob* parent, const InputList& inputs):
            Job(parent, "ProcessInputListJob"),
            inputs_(inputs){}
        ~ProcessInputListJob() = default;
    };

    JobResult ProcessTransactionInputsJob::DoWork(){
        JobPoolWorker* worker = JobScheduler::GetThreadWorker();

        TransactionPtr tx = GetTransaction();
        InputList& inputs = tx->inputs();
        auto start = inputs.begin();
        auto end = inputs.end();
        while(start != end){
            auto next = std::distance(start, end) > ProcessInputListJob::kMaxNumberOfInputs
                      ? start + ProcessInputListJob::kMaxNumberOfInputs
                      : end;

            InputList chunk(start, next);
            ProcessInputListJob* job = new ProcessInputListJob(this, chunk);
            if(!worker->Submit(job))
                return Failed("Cannot schedule ProcessInputListJob()");
            start = next;
        }

        return Success("done.");
    }

    class ProcessOutputListJob : public Job{
    public:
        static const int64_t kMaxNumberOfOutputs = 128;
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
            for(auto& it : outputs_){
                UnclaimedTransactionPtr utxo = CreateUnclaimedTransaction(it);
                Hash hash = utxo->GetHash();
                if(!UnclaimedTransactionPool::PutUnclaimedTransaction(hash, utxo)){
                    std::stringstream ss;
                    ss << "Couldn't create unclaimed transaction: " << hash;
                    return Failed(ss.str());
                }
            }
            return Success("done.");
        }
    public:
        ProcessOutputListJob(ProcessTransactionOutputsJob* parent, int64_t wid, const OutputList& outputs):
            Job(parent, "ProcessOutputListJob"),
            worker_(wid),
            offset_(wid * ProcessOutputListJob::kMaxNumberOfOutputs),
            outputs_(outputs){}
        ~ProcessOutputListJob() = default;

        TransactionPtr GetTransaction() const{
            return ((ProcessTransactionOutputsJob*)GetParent())->GetTransaction();
        }
    };

    JobResult ProcessTransactionOutputsJob::DoWork(){
        JobPoolWorker* worker = JobScheduler::GetThreadWorker();
        TransactionPtr tx = GetTransaction();

        int64_t nworker = 0;
        OutputList& outputs = tx->outputs();
        auto start = outputs.begin();
        auto end = outputs.end();
        while(start != end){
            auto next = std::distance(start, end) > ProcessOutputListJob::kMaxNumberOfOutputs
                        ? start + ProcessOutputListJob::kMaxNumberOfOutputs
                        : end;

            OutputList chunk(start, next);
            ProcessOutputListJob* job = new ProcessOutputListJob(this, nworker++, chunk);
            if(!worker->Submit(job))
                return Failed("Cannot schedule ProcessOutputListJob()");

            start = next;
        }

        return Success("done.");
    }

    JobResult ProcessTransactionJob::DoWork(){
        JobPoolWorker* worker = JobScheduler::GetThreadWorker();
        ProcessTransactionInputsJob* process_inputs = new ProcessTransactionInputsJob(this);
        worker->Submit(process_inputs);
        worker->Wait(process_inputs);

        ProcessTransactionOutputsJob* process_outputs = new ProcessTransactionOutputsJob(this);
        worker->Submit(process_outputs);
        worker->Wait(process_outputs);
        return Success("Finished.");
    }
}