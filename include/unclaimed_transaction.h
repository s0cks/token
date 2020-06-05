#ifndef TOKEN_UNCLAIMED_TRANSACTION_H
#define TOKEN_UNCLAIMED_TRANSACTION_H

#include "common.h"
#include "object.h"
#include "pool.h"

namespace Token{
    class Transaction;
    class Output;

    class UnclaimedTransaction : public BinaryObject{
    private:
        uint256_t hash_; // Transaction Hash
        uint32_t index_; // Output Index
    protected:
        bool GetBytes(CryptoPP::SecByteBlock& bytes) const;
    public:
        typedef Proto::BlockChain::UnclaimedTransaction RawType;

        UnclaimedTransaction():
            hash_(),
            index_(){}
        UnclaimedTransaction(const uint256_t& hash, uint32_t index):
            hash_(hash),
            index_(index){}
        UnclaimedTransaction(const Transaction& tx, uint32_t index):
            hash_(tx.GetHash()),
            index_(index){}
        UnclaimedTransaction(const RawType& raw):
            hash_(HashFromHexString(raw.tx_hash())),
            index_(raw.tx_index()){}
        UnclaimedTransaction(const UnclaimedTransaction& other):
            hash_(other.hash_),
            index_(other.index_){}
        ~UnclaimedTransaction(){}

        uint256_t GetTransaction() const{
            return hash_;
        }

        uint32_t GetIndex() const{
            return index_;
        }

        bool GetTransaction(Transaction* result) const;
        bool GetOutput(Output* result) const;

        friend bool operator==(const UnclaimedTransaction& lhs, const UnclaimedTransaction& rhs){
            return lhs.GetHash() == rhs.GetHash();
        }

        friend bool operator!=(const UnclaimedTransaction& lhs, const UnclaimedTransaction& rhs){
            return !operator==(lhs, rhs);
        }

        friend RawType& operator<<(RawType& stream, const UnclaimedTransaction& utxo){
            stream.set_tx_hash(HexString(utxo.hash_));
            stream.set_tx_index(utxo.index_);
            return stream;
        }

        UnclaimedTransaction& operator=(const UnclaimedTransaction& other){
            hash_ = other.hash_;
            index_ = other.index_;
            return (*this);
        }
    };

    class UnclaimedTransactionPoolVisitor;
    class UnclaimedTransactionPool : public IndexManagedPool<UnclaimedTransaction, UnclaimedTransaction::RawType>{
    private:
        static UnclaimedTransactionPool* GetInstance();

        std::string CreateObjectLocation(const uint256_t& hash, UnclaimedTransaction* value) const{
            if(ContainsObject(hash)){
                return GetObjectLocation(hash);
            }

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

        static bool Initialize();
        static bool PutUnclaimedTransaction(UnclaimedTransaction* utxo);
        static bool HasUnclaimedTransaction(const uint256_t& hash);
        static bool RemoveUnclaimedTransaction(const uint256_t& hash);
        static bool Accept(UnclaimedTransactionPoolVisitor* vis);
        static bool GetUnclaimedTransactions(std::vector<uint256_t>& utxos);
        static bool GetUnclaimedTransactions(const std::string& user, std::vector<uint256_t>& utxos);
        static UnclaimedTransaction* GetUnclaimedTransaction(const uint256_t& hash);
    };

    class UnclaimedTransactionPoolVisitor{
    public:
        virtual ~UnclaimedTransactionPoolVisitor() = default;

        virtual bool VisitStart() { return true; }
        virtual bool Visit(UnclaimedTransaction* utxo) = 0;
        virtual bool VisitEnd() { return true; };
    };
}

#endif //TOKEN_UNCLAIMED_TRANSACTION_H