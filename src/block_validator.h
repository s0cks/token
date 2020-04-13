#ifndef TOKEN_BLOCK_VALIDATOR_H
#define TOKEN_BLOCK_VALIDATOR_H

#include <glog/logging.h>
#include "block.h"
#include "unclaimed_transaction.h"

namespace Token{
    class BlockValidator : public BlockVisitor{
    private:
        std::vector<Transaction*> valid_txs_;
        std::vector<Transaction*> invalid_txs_;
    public:
        BlockValidator():
                valid_txs_(),
                invalid_txs_(){}
        ~BlockValidator(){}

        bool IsValid(Transaction* tx){
            if(!tx->IsSigned()){
                LOG(WARNING) << "transaction is not signed: " << tx->GetHash();
                return false;
            } else if(tx->GetNumberOfInputs() <= 0){
                LOG(WARNING) << "transaction " << tx->GetHash() << " has no inputs";
                return false;
            } else if(tx->GetNumberOfOutputs() <= 0){
                LOG(WARNING) << "transaction " << tx->GetHash() << " has no outputs";
                return false;
            }

            int i;
            for(i = 0; i < tx->GetNumberOfInputs(); i++){
                Input* in;
                if(!(in = tx->GetInput(i))){
                    LOG(WARNING) << "couldn't get input #" << i;
                    return false;
                }
                UnclaimedTransaction utxo(in->GetOutput());
                /*
                 * if(!UnclaimedTransactionPool::HasUnclaimedTransaction(utxo)){
                    LOG(WARNING) << "no unclaimed transaction for input #" << i;
                    return false;
                }
                 */
            }
            return true;
        }

        bool VisitBlockStart(){
            return true;
        }

        bool VisitBlockEnd(){
            return true;
        }

        bool VisitTransaction(Transaction* tx){
            if(IsValid(tx)){
                int i;
                for(i = 0; i < tx->GetNumberOfOutputs(); i++){
                    Output* out;
                    if(!(out = tx->GetOutput(i))){
                        LOG(WARNING) << "couldn't get output #" << i;
                        return false;
                    }

                    UnclaimedTransaction utxo(out);
                    /*
                     * if(!UnclaimedTransactionPool::PutUnclaimedTransaction(utxo)){
                        LOG(WARNING) << "couldn't create new unclaimed transaction: " << utxo.GetHash();
                        LOG(WARNING) << "*** Unclaimed Transaction: ";
                        LOG(WARNING) << "***   + Input: " << utxo.GetTransactionHash() << "[" << utxo.GetIndex() << "]";
                        LOG(WARNING) << "***   + Output: " << utxo.GetToken() << "(" << utxo.GetUser() << ")";
                        return false;
                    }
                     */
                }
                for(i = 0; i < tx->GetNumberOfInputs(); i++){
                    Input* input;
                    if(!(input = tx->GetInput(i))){
                        LOG(WARNING) << "couldn't get input #" << i;
                        return false;
                    }

                    UnclaimedTransaction utxo(input->GetOutput());
                    /*
                     * if(!UnclaimedTransactionPool::GetInstance()->RemoveUnclaimedTransaction(utxo)){
                        LOG(WARNING) << "couldn't remove unclaimed transaction: " << utxo.GetHash();
                        LOG(WARNING) << "*** Unclaimed Transaction: ";
                        LOG(WARNING) << "***   + Input: " << utxo.GetTransactionHash() << "[" << utxo.GetIndex() << "]";
                        LOG(WARNING) << "***   + Output: " << utxo.GetToken() << "(" << utxo.GetUser() << ")";
                        return false;
                    }
                     */
                }


                valid_txs_.push_back(tx);
            } else{
                invalid_txs_.push_back(tx);
            }
            return true;
        }

        std::vector<Transaction*> GetValidTransactions(){
            return valid_txs_;
        }

        std::vector<Transaction*> GetInvalidTransactions(){
            return invalid_txs_;
        }
    };
}

#endif //TOKEN_BLOCK_VALIDATOR_H