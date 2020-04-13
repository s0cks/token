#ifndef TOKEN_UNCLAIMED_TRANSACTION_H
#define TOKEN_UNCLAIMED_TRANSACTION_H

#include <map>
#include <string>
#include <leveldb/db.h>
#include "pool.h"
#include "transaction.h"
#include "service/service.h"

namespace Token{
    class UnclaimedTransaction : public BinaryObject{
    private:
        Output* output_;
    protected:
        bool GetBytes(CryptoPP::SecByteBlock& bytes) const;
    public:
        UnclaimedTransaction():
            output_(nullptr){}
        UnclaimedTransaction(Output* output):
            output_(output){}
        UnclaimedTransaction(const Proto::BlockChain::UnclaimedTransaction& raw):
            output_(nullptr){}
        ~UnclaimedTransaction(){}

        Output* GetOutput() const{
            return output_;
        }

        Transaction* GetTransaction() const{
            return GetOutput()->GetTransaction();
        }

        std::string GetTransactionHash() const{
            return HexString(GetTransaction()->GetHash());
        }

        std::string GetUser() const{
            return GetOutput()->GetUser();
        }

        std::string GetToken() const{
            return GetOutput()->GetToken();
        }

        uint32_t GetIndex() const{
            return GetOutput()->GetIndex();
        }

        friend bool operator==(const UnclaimedTransaction& lhs, const UnclaimedTransaction& rhs){
            return lhs.GetHash() == rhs.GetHash();
        }

        friend bool operator!=(const UnclaimedTransaction& lhs, const UnclaimedTransaction& rhs){
            return !operator==(lhs, rhs);
        }

        friend Proto::BlockChain::UnclaimedTransaction& operator<<(Proto::BlockChain::UnclaimedTransaction& stream, const UnclaimedTransaction& utxo){
            stream.set_tx_hash(utxo.GetTransactionHash());
            stream.set_tx_index(utxo.GetIndex());
            stream.set_user(utxo.GetUser());
            stream.set_token(utxo.GetToken());
            return stream;
        }

        UnclaimedTransaction& operator=(const UnclaimedTransaction& other){
            output_ = other.output_;
            return (*this);
        }
    };

    class UnclaimedTransactionPool : public IndexManagedPool{
    private:
        static UnclaimedTransactionPool* GetInstance();
        static bool LoadUnclaimedTransaction(const std::string& filename, UnclaimedTransaction* utxo);
        static bool SaveUnclaimedTransaction(const std::string& filename, UnclaimedTransaction* utxo);

        UnclaimedTransactionPool(): IndexManagedPool(FLAGS_path + "/utxos"){}
    public:
        ~UnclaimedTransactionPool(){}

        static UnclaimedTransaction* GetUnclaimedTransaction(const uint256_t& hash);
        static UnclaimedTransaction* RemoveUnclaimedTransaction(const uint256_t& hash);
        static bool Initialize();
        static bool PutUnclaimedTransaction(UnclaimedTransaction* utxo);
        static bool HasUnclaimedTransaction(const uint256_t& hash);
    };
}

#endif //TOKEN_UNCLAIMED_TRANSACTION_H