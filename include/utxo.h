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
    class UnclaimedTransaction{
    private:
        Messages::UnclaimedTransaction* raw_;

        inline Messages::UnclaimedTransaction*
        GetRaw() const{
            return raw_;
        }

        void Encode(ByteBuffer* bb) const;
    public:
        UnclaimedTransaction(const UnclaimedTransaction& other):
            raw_(new Messages::UnclaimedTransaction()){
            GetRaw()->set_tx_hash(other.GetTransactionHash());
            GetRaw()->set_index(other.GetIndex());
            GetRaw()->set_user(other.GetUser());
            GetRaw()->set_token(other.GetToken());
        }
        UnclaimedTransaction(const std::string& hash, uint32_t idx, const std::string& user, const std::string& token):
            raw_(new Messages::UnclaimedTransaction()){
            GetRaw()->set_tx_hash(hash);
            GetRaw()->set_index(idx);
            GetRaw()->set_user(user);
            GetRaw()->set_token(token);
        }
        UnclaimedTransaction(const std::string& hash, uint32_t idx, Output* out):
            raw_(new Messages::UnclaimedTransaction()){
            GetRaw()->set_tx_hash(hash);
            GetRaw()->set_index(idx);
            GetRaw()->set_user(out->GetUser());
            GetRaw()->set_token(out->GetToken());
        }
        UnclaimedTransaction(const std::string& hash, uint32_t idx):
            raw_(new Messages::UnclaimedTransaction()){
            GetRaw()->set_tx_hash(hash);
            GetRaw()->set_index(idx);
        }
        UnclaimedTransaction():
            raw_(new Messages::UnclaimedTransaction()){}
        ~UnclaimedTransaction(){}

        std::string GetHash() const;

        std::string GetTransactionHash() const{
            return GetRaw()->tx_hash();
        }

        std::string GetUser() const{
            return GetRaw()->user();
        }

        std::string GetToken() const{
            return GetRaw()->token();
        }

        uint32_t GetIndex() const{
            return GetRaw()->index();
        }

        friend bool operator==(const UnclaimedTransaction& lhs, const UnclaimedTransaction& rhs){
            return lhs.GetTransactionHash() == rhs.GetTransactionHash() &&
                    lhs.GetIndex() == rhs.GetIndex();
        }

        bool operator<(const UnclaimedTransaction& other) const{
            return GetHash() < other.GetHash() || (GetHash() == other.GetHash() && GetIndex() < other.GetIndex());
        }

        friend std::ostream& operator<<(std::ostream& stream, const UnclaimedTransaction& rhs){
            stream << rhs.GetTransactionHash() << "[" << rhs.GetIndex() << "] => " << rhs.GetToken() << "(" << rhs.GetUser() << ")";
            return stream;
        }

        UnclaimedTransaction& operator=(const UnclaimedTransaction& other){
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

        UnclaimedTransactionPool();
    public:
        ~UnclaimedTransactionPool();

        bool GetUnclaimedTransactions(const std::string& user, std::vector<UnclaimedTransaction*>& utxos);
        bool GetUnclaimedTransactions(std::vector<UnclaimedTransaction*>& utxos);
        bool GetUnclaimedTransactions(const std::string& user, Messages::UnclaimedTransactionsList* utxos);
        bool GetUnclaimedTransactions(Messages::UnclaimedTransactionsList* utxos);
        bool GetUnclaimedTransaction(const std::string& tx_hash, int index, UnclaimedTransaction** result);
        bool RemoveUnclaimedTransaction(UnclaimedTransaction* utxo);
        bool AddUnclaimedTransaction(UnclaimedTransaction* utxo);
        bool ClearUnclaimedTransactions();

        static UnclaimedTransactionPool* GetInstance();
        static bool LoadUnclaimedTransactionPool(const std::string& filename);
    };
}

#endif //TOKEN_UTXO_H