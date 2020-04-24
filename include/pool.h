#ifndef TOKEN_POOL_H
#define TOKEN_POOL_H

#include <leveldb/db.h>
#include "common.h"
#include "uint256_t.h"

namespace Token{
    class Transaction;
    class UnclaimedTransaction;

    template<typename PoolObject>
    class IndexManagedPool{
    private:
        std::string path_;
        leveldb::DB* index_;
        bool initialized_;
    protected:
        leveldb::DB* GetIndex() const{
            return index_;
        }

        virtual std::string CreateObjectLocation(const uint256_t& hash, PoolObject* value) const = 0;

        bool InitializeIndex(){
            leveldb::Options options;
            options.create_if_missing = true;
            return initialized_ = leveldb::DB::Open(options, GetRoot() + "/index", &index_).ok();
        }

        bool ContainsObject(const uint256_t& hash) const{
            leveldb::ReadOptions readOpts;
            std::string key = HexString(hash);
            std::string value;
            return GetIndex()->Get(readOpts, key, &value).ok();
        }

        bool LoadRawObject(const std::string& filename, PoolObject* result) const{
            std::fstream fd(filename, std::ios::in|std::ios::binary);
            typename PoolObject::RawType raw;
            if(!raw.ParseFromIstream(&fd)) return false;
            (*result) = PoolObject(raw);
            return true;
        }

        bool LoadObject(const uint256_t& hash, PoolObject* result) const{
            leveldb::ReadOptions readOpts;
            std::string key = HexString(hash);
            std::string location;
            if(!GetIndex()->Get(readOpts, key, &location).ok()) return false;
            return LoadRawObject(location, result);
        }

        bool SaveRawObject(const std::string& filename, PoolObject* value) const{
            std::fstream fd(filename, std::ios::out|std::ios::binary);
            typename PoolObject::RawType raw;
            raw << (*value);
            if(!raw.SerializeToOstream(&fd)) return false;
            return true;
        }

        bool SaveObject(const uint256_t& hash, PoolObject* value) const{
            if(ContainsObject(hash)) return false;
            leveldb::WriteOptions writeOpts;
            std::string key = HexString(hash);
            std::string location = CreateObjectLocation(hash, value);
            if(!GetIndex()->Put(writeOpts, key, location).ok()) return false;
            return SaveRawObject(location, value);
        }

        bool DeleteRawObject(const std::string& filename) const{
            return std::remove(filename.c_str()) == 0;
        }

        bool DeleteObject(const uint256_t& hash) const{
            std::string key = HexString(hash);
            std::string location;
            leveldb::ReadOptions readOpts;
            if(!GetIndex()->Get(readOpts, key, &location).ok()) return false;
            leveldb::WriteOptions writeOpts;
            if(!GetIndex()->Delete(writeOpts, key).ok()) return false;
            return DeleteRawObject(location);
        }

        bool IsInitialized() const{
            return initialized_;
        }

        IndexManagedPool(const std::string& path):
            path_(path),
            index_(nullptr),
            initialized_(false){}
    public:
        virtual ~IndexManagedPool() = default;

        std::string GetRoot() const{
            return path_;
        }
    };

    class TransactionPool : public IndexManagedPool<Transaction>{
    private:
        pthread_rwlock_t rwlock_;

        static TransactionPool* GetInstance();

        std::string CreateObjectLocation(const uint256_t& hash, Transaction* tx) const{
            std::string hashString = HexString(hash);
            std::string front = hashString.substr(0, 8);
            std::string tail = hashString.substr(hashString.length() - 8, hashString.length());

            std::string filename = GetRoot() + "/" + front + ".dat";
            if(FileExists(filename)){
                filename = GetRoot() + "/" + tail + ".dat";
            }
            return filename;
        }

        TransactionPool():
                rwlock_(),
                IndexManagedPool(FLAGS_path + "/txs"){
            pthread_rwlock_init(&rwlock_, NULL);
        }
    public:
        ~TransactionPool(){}

        static bool Initialize();
        static bool PutTransaction(Transaction* tx);
        static bool GetTransaction(const uint256_t& hash, Transaction* result);
        static bool GetTransactions(std::vector<Transaction>& txs);
        static bool RemoveTransaction(const uint256_t& hash);
        static uint64_t GetSize();
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

#endif //TOKEN_POOL_H