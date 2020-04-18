#ifndef TOKEN_UNCLAIMED_TRANSACTION_H
#define TOKEN_UNCLAIMED_TRANSACTION_H

#include <map>
#include <string>
#include <leveldb/db.h>
#include "blockchain.pb.h"
#include "pool.h"
#include "transaction.h"
#include "service/service.h"

namespace Token{
    class UnclaimedTransaction : public BinaryObject{
    private:
        uint256_t tx_hash_;
        uint64_t tx_index_;
        std::string user_;
        std::string token_;
    protected:
        bool GetBytes(CryptoPP::SecByteBlock& bytes) const;
    public:
        typedef Proto::BlockChain::UnclaimedTransaction RawType;

        UnclaimedTransaction():
            tx_hash_(),
            tx_index_(0),
            user_(),
            token_(){}
        UnclaimedTransaction(const uint256_t& tx_hash, uint64_t idx, std::string user, std::string token):
            tx_hash_(tx_hash),
            tx_index_(idx),
            user_(user),
            token_(token){}
        UnclaimedTransaction(const Transaction& tx, uint64_t idx):
            tx_hash_(tx.GetHash()),
            tx_index_(idx),
            user_(),
            token_(){
            Output out;
            if(!tx.GetOutput(idx, &out)){
                tx_hash_ = uint256_t();
                tx_index_ = 0;
                return;
            }
            user_ = out.GetUser();
            token_ = out.GetToken();
        }
        UnclaimedTransaction(const Transaction& tx, const Output& out):
            tx_hash_(tx.GetHash()),
            tx_index_(tx.GetIndex()),
            user_(out.GetUser()),
            token_(out.GetToken()){}
        UnclaimedTransaction(const RawType& raw):
            tx_hash_(HashFromHexString(raw.tx_hash())),
            tx_index_(raw.tx_index()),
            user_(raw.user()),
            token_(raw.token()){}
        ~UnclaimedTransaction(){}

        uint256_t GetTransaction() const{
            return tx_hash_;
        }

        uint64_t GetIndex() const{
            return tx_index_;
        }

        std::string GetUser() const{
            return user_;
        }

        std::string GetToken() const{
            return token_;
        }

        friend bool operator==(const UnclaimedTransaction& lhs, const UnclaimedTransaction& rhs){
            return lhs.GetHash() == rhs.GetHash();
        }

        friend bool operator!=(const UnclaimedTransaction& lhs, const UnclaimedTransaction& rhs){
            return !operator==(lhs, rhs);
        }

        friend RawType& operator<<(RawType& stream, const UnclaimedTransaction& utxo){
            stream.set_tx_hash(HexString(utxo.GetTransaction()));
            stream.set_tx_index(utxo.GetIndex());
            stream.set_user(utxo.GetUser());
            stream.set_token(utxo.GetToken());
            return stream;
        }

        UnclaimedTransaction& operator=(const UnclaimedTransaction& other){
            tx_hash_ = other.tx_hash_;
            tx_index_ = other.tx_index_;
            user_ = other.user_;
            token_ = other.token_;
            return (*this);
        }
    };

    class UnclaimedTransactionPool : public IndexManagedPool<UnclaimedTransaction>{
    private:
        static UnclaimedTransactionPool* GetInstance();

        std::string CreateObjectLocation(const uint256_t& hash, UnclaimedTransaction* value) const{
            std::string hashString = HexString(hash);
            std::string front = hashString.substr(0, 8);
            std::string tail = hashString.substr(hashString.length() - 8, hashString.length());
            std::string filename = GetRoot() + "/" + front + ".dat";
            if(FileExists(filename)){
                filename = GetRoot() + "/" + tail + ".dat";
            }
            return filename;
        }

        UnclaimedTransactionPool(): IndexManagedPool(FLAGS_path + "/utxos"){}
    public:
        ~UnclaimedTransactionPool(){}

        static bool RemoveUnclaimedTransaction(const uint256_t& hash);
        static bool Initialize();
        static bool PutUnclaimedTransaction(UnclaimedTransaction* utxo);
        static bool GetUnclaimedTransaction(const uint256_t& hash, UnclaimedTransaction* result);
        static bool GetUnclaimedTransactions(std::vector<uint256_t>& utxos);
        static bool GetUnclaimedTransactions(const std::string& user, std::vector<uint256_t>& utxos);
    };
}

#endif //TOKEN_UNCLAIMED_TRANSACTION_H