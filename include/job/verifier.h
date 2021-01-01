#ifndef TOKEN_VERIFIER_H
#define TOKEN_VERIFIER_H

#include "job.h"
#include "block.h"
#include "transaction.h"
#include "utils/atomic_linked_list.h"

namespace Token{
    class VerifierJob : public Job{
    protected:
        VerifierJob(Job* parent, const std::string& name):
            Job(parent, name){}
    public:
        virtual ~VerifierJob() = default;
    };

    class VerifyBlockJob;

    class VerifyTransactionJob : public VerifierJob{
    private:
        TransactionPtr transaction_;
    protected:
        JobResult DoWork();
    public:
        VerifyTransactionJob(VerifyBlockJob* parent, const TransactionPtr& tx);
        ~VerifyTransactionJob() = default;
    };

    class VerifyBlockJob : public VerifierJob{
    private:
        BlockPtr block_;
        AtomicLinkedList<Hash> valid_;
        AtomicLinkedList<Hash> invalid_;
    protected:
        JobResult DoWork();
    public:
        VerifyBlockJob(const BlockPtr& blk):
            VerifierJob(nullptr, "VerifyBlock"),
            block_(blk){}
        ~VerifyBlockJob() = default;

        BlockPtr& GetBlock(){
            return block_;
        }

        BlockPtr GetBlock() const{
            return block_;
        }

        void AddValidTransaction(const Hash& hash){
            valid_.PushBack(hash);
        }

        void AddInvalidTransaction(const Hash& hash){
            invalid_.PushBack(hash);
        }
    };
}

#endif //TOKEN_VERIFIER_H