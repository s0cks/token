#ifndef TOKEN_UTXO_H
#define TOKEN_UTXO_H

#include <string>
#include <map>
#include "common.h"

namespace Token{
    class UnclaimedTransaction{
    private:
        std::string hash_;
        uint32_t index_;
    public:
        UnclaimedTransaction(const std::string& hash, uint32_t idx):
            hash_(hash),
            index_(idx){}
        ~UnclaimedTransaction(){}

        std::string GetHash() const{
            return hash_;
        }

        uint32_t GetIndex() const{
            return index_;
        }

        friend bool operator==(const UnclaimedTransaction& lhs, const UnclaimedTransaction& rhs){
            return lhs.GetIndex() == rhs.GetIndex() &&
                    lhs.GetHash() == rhs.GetHash();
        }

        bool operator<(const UnclaimedTransaction& other) const{
            return GetHash() < other.GetHash() || (GetHash() == other.GetHash() && GetIndex() < other.GetIndex());
        }

        friend std::ostream& operator<<(std::ostream& stream, const UnclaimedTransaction& rhs){
            stream << rhs.GetHash() << "[" << rhs.GetIndex() << "]";
            return stream;
        }

        UnclaimedTransaction& operator=(const UnclaimedTransaction& other){
            hash_ = other.hash_;
            index_ = other.index_;
            return *this;
        }
    };

    class Output;

    class UnclaimedTransactionPool{
    private:
        std::map<UnclaimedTransaction, Output*> utxos_;
    public:
        UnclaimedTransactionPool():
            utxos_(){}
        UnclaimedTransactionPool(const UnclaimedTransactionPool& other):
            utxos_(){
            utxos_ = other.utxos_;
        }
        ~UnclaimedTransactionPool(){}

        void Insert(UnclaimedTransaction tx, Output* out){
            utxos_.insert({ tx, out });
        }

        void Remove(UnclaimedTransaction tx){
            utxos_.erase(tx);
        }

        Output* Get(UnclaimedTransaction tx){
            return utxos_.find(tx)->second;
        }

        bool Contains(UnclaimedTransaction tx){
            return utxos_.find(tx) != utxos_.end();
        }

        bool GetUnclaimedTransactions(std::vector<UnclaimedTransaction*> utxos) const {
            for(auto& it : utxos_){
                UnclaimedTransaction utxo = it.first;
                utxos.push_back(&utxo);
            }
            return true;
        }

        size_t Size() const{
            return utxos_.size();
        }

        bool IsEmpty(){
            return utxos_.empty();
        }

        friend std::ostream& operator<<(std::ostream& stream, const UnclaimedTransactionPool& rhs) {
            stream << "Unclaimed Transactions:" << std::endl;
            for (auto it : rhs.utxos_) {
                stream << "\t+ " << it.first << " => " << it.second->GetUser() << "#" << it.second->GetToken()
                       << std::endl;
            }
            return stream;
        }

        UnclaimedTransactionPool& operator=(const UnclaimedTransactionPool& other){
            utxos_ = other.utxos_;
            return *this;
        }
    };
}

#endif //TOKEN_UTXO_H