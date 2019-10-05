#ifndef TOKEN_UTXO_H
#define TOKEN_UTXO_H

#include <string>
#include <map>
#include <service.pb.h>
#include <sqlite3.h>
#include "common.h"
#include "transaction.h"
#include "pthread.h"

namespace Token{
    class UnclaimedTransaction : public AllocatorObject{
    private:
        Messages::UnclaimedTransaction raw_;

        Messages::UnclaimedTransaction*
        GetRaw(){
            return &raw_;
        }
    public:
        UnclaimedTransaction(UnclaimedTransaction& other):
            raw_(){
            GetRaw()->set_tx_hash(other.GetTransactionHash());
            GetRaw()->set_index(other.GetIndex());
            GetRaw()->set_user(other.GetUser());
            GetRaw()->set_token(other.GetToken());
        }
        UnclaimedTransaction(const std::string& hash, uint32_t idx, const std::string& user, const std::string& token):
            raw_(){
            GetRaw()->set_tx_hash(hash);
            GetRaw()->set_index(idx);
            GetRaw()->set_user(user);
            GetRaw()->set_token(token);
        }
        UnclaimedTransaction(const std::string& hash, uint32_t idx, Output* out):
            raw_(){
            GetRaw()->set_tx_hash(hash);
            GetRaw()->set_index(idx);
            GetRaw()->set_user(out->GetUser());
            GetRaw()->set_token(out->GetToken());
        }
        UnclaimedTransaction(const std::string& hash, uint32_t idx):
            raw_(){
            GetRaw()->set_tx_hash(hash);
            GetRaw()->set_index(idx);
        }
        UnclaimedTransaction(const std::string& utxo_hash);
        UnclaimedTransaction():
            raw_(){}
        ~UnclaimedTransaction(){}

        std::string ToString();
        std::string GetHash();

        std::string GetTransactionHash(){
            return GetRaw()->tx_hash();
        }

        std::string GetUser(){
            return GetRaw()->user();
        }

        std::string GetToken(){
            return GetRaw()->token();
        }

        uint32_t GetIndex(){
            return GetRaw()->index();
        }

        void* operator new(size_t size);

        friend bool operator==(UnclaimedTransaction& lhs, UnclaimedTransaction& rhs){
            return lhs.GetTransactionHash() == rhs.GetTransactionHash() &&
                    lhs.GetIndex() == rhs.GetIndex();
        }

        bool operator<(UnclaimedTransaction& other){
            return GetHash() < other.GetHash() || (GetHash() == other.GetHash() && GetIndex() < other.GetIndex());
        }

        friend std::ostream& operator<<(std::ostream& stream, UnclaimedTransaction& rhs){
            stream << rhs.GetTransactionHash() << "[" << rhs.GetIndex() << "] => " << rhs.GetToken() << "(" << rhs.GetUser() << ")";
            return stream;
        }

        UnclaimedTransaction operator=(UnclaimedTransaction other){
            GetRaw()->set_tx_hash(other.GetTransactionHash());
            GetRaw()->set_index(other.GetIndex());
            GetRaw()->set_user(other.GetUser());
            GetRaw()->set_token(other.GetToken());
            return *this;
        }
    };

    class Output;

    class UnclaimedTransactionPool{
    private:
        std::string db_path_;
        sqlite3* database_;
        pthread_rwlock_t rwlock_;

        bool LoadPool(const std::string& filename);

        UnclaimedTransactionPool();
    public:
        ~UnclaimedTransactionPool();

        bool GetUnclaimedTransactions(const std::string& user, std::vector<std::string>& utxos);
        bool GetUnclaimedTransactions(std::vector<std::string>& utxos);
        bool GetUnclaimedTransactions(const std::string& user, Messages::UnclaimedTransactionList* utxos);
        bool GetUnclaimedTransactions(Messages::UnclaimedTransactionList* utxos);
        bool GetUnclaimedTransaction(const std::string& tx_hash, int index, UnclaimedTransaction* result);
        bool GetUnclaimedTransaction(const std::string& hash, UnclaimedTransaction* result);
        bool RemoveUnclaimedTransaction(UnclaimedTransaction* utxo);
        bool AddUnclaimedTransaction(UnclaimedTransaction* utxo);
        // bool ClearUnclaimedTransactions();

        static UnclaimedTransactionPool* GetInstance();
        static bool LoadUnclaimedTransactionPool(const std::string& filename);
    };

    class UnclaimedTransactionPoolPrinter{
    private:
        UnclaimedTransactionPoolPrinter(){}
    public:
        ~UnclaimedTransactionPoolPrinter(){}

        static void Print();
    };
}

#endif //TOKEN_UTXO_H