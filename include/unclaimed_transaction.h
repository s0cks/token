#ifndef TOKEN_UNCLAIMED_TRANSACTION_H
#define TOKEN_UNCLAIMED_TRANSACTION_H

#include "common.h"
#include "object.h"
#include "pool.h"

namespace Token{
    class Transaction;
    class Output;

    class UnclaimedTransaction : public BinaryObject<Proto::BlockChain::UnclaimedTransaction>{
    public:
        typedef Proto::BlockChain::UnclaimedTransaction RawType;
    private:
        uint256_t hash_;
        uint32_t index_;
        std::string user_;

        UnclaimedTransaction(const uint256_t& hash, uint32_t idx, const std::string& user):
            hash_(hash),
            user_(user),
            index_(idx){}

        bool Encode(RawType& raw) const;

        friend class IndexManagedPool<UnclaimedTransaction, RawType>;
        friend class UnclaimedTransactionPool;
    public:
        ~UnclaimedTransaction(){}

        uint256_t GetTransaction() const{
            return hash_;
        }

        uint32_t GetIndex() const{
            return index_;
        }

        std::string GetUser() const{
            return user_;
        }

        std::string ToString() const;

        static UnclaimedTransaction* NewInstance(const uint256_t& hash, uint32_t index, const std::string& user);
        static UnclaimedTransaction* NewInstance(const RawType& raw);
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
        static UnclaimedTransaction* GetUnclaimedTransaction(const uint256_t& tx_hash, uint32_t tx_index);
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