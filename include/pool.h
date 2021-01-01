#ifndef TOKEN_POOL_H
#define TOKEN_POOL_H

#include <leveldb/db.h>
#include <leveldb/slice.h>
#include <leveldb/comparator.h>
#include <leveldb/write_batch.h>
#include "hash.h"
#include "block.h"
#include "transaction.h"
#include "unclaimed_transaction.h"
#include "utils/json_conversion.h"

namespace Token{
    class ObjectPoolVisitor{
    protected:
        ObjectPoolVisitor() = default;
    public:
        virtual ~ObjectPoolVisitor() = default;
    };

    class ObjectPoolBlockVisitor : ObjectPoolVisitor{
    protected:
        ObjectPoolBlockVisitor() = default;
    public:
        virtual ~ObjectPoolBlockVisitor() = default;
        virtual bool Visit(const BlockPtr& blk) const = 0;
    };

    class ObjectPoolTransactionVisitor : ObjectPoolVisitor{
    protected:
        ObjectPoolTransactionVisitor() = default;
    public:
        virtual ~ObjectPoolTransactionVisitor() = default;
        virtual bool Visit(const TransactionPtr& tx) const = 0;
    };

    class ObjectPoolUnclaimedTransactionVisitor : ObjectPoolVisitor{
    protected:
        ObjectPoolUnclaimedTransactionVisitor() = default;
    public:
        virtual ~ObjectPoolUnclaimedTransactionVisitor() = default;
        virtual bool Visit(const UnclaimedTransactionPtr& utxo) const = 0;
    };

#define FOR_EACH_POOL_STATE(V) \
    V(Uninitialized)           \
    V(Initializing)            \
    V(Initialized)

#define FOR_EACH_POOL_STATUS(V) \
    V(Ok)                       \
    V(Warning)                  \
    V(Error)

    class ObjectPoolKey{
    public:
        static const int64_t kSize = sizeof(int32_t)
                                   + Hash::kSize;
    private:
        ObjectTag tag_;
        Hash hash_;
    public:
        ObjectPoolKey():
            tag_(),
            hash_(){}
        ObjectPoolKey(const ObjectTag& tag, const Hash& hash):
            tag_(tag),
            hash_(hash){}
        ObjectPoolKey(const ObjectTag::Type& tag, const Hash& hash):
            tag_(tag),
            hash_(hash){}
        ObjectPoolKey(const BufferPtr& buff):
            tag_(static_cast<ObjectTag>(buff->GetInt())),
            hash_(buff->GetHash()){}
        ObjectPoolKey(const leveldb::Slice& slice):
            tag_(),
            hash_(){
            BufferPtr buff = Buffer::From(slice.data(), slice.size());
            tag_ = buff->GetObjectTag();
            hash_ = buff->GetHash();
        }
        ~ObjectPoolKey() = default;

        ObjectTag& GetTag(){
            return tag_;
        }

        ObjectTag GetTag() const{
            return tag_;
        }

        Hash& GetHash(){
            return hash_;
        }

        Hash GetHash() const{
            return hash_;
        }

        bool IsUnclaimedTransaction() const{
            return tag_.IsUnclaimedTransaction();
        }

        bool IsTransaction() const{
            return tag_.IsTransaction();
        }

        bool IsBlock() const{
            return tag_.IsBlock();
        }

        bool Write(const BufferPtr& buff) const{
            buff->PutObjectTag(tag_);
            buff->PutHash(hash_);
            return true;
        }

        void operator=(const ObjectPoolKey& key){
            tag_ = key.tag_;
            hash_ = key.hash_;
        }

        friend bool operator==(const ObjectPoolKey& a, const ObjectPoolKey& b){
            return a.tag_ == b.tag_
                && a.hash_ == b.hash_;
        }

        friend bool operator!=(const ObjectPoolKey& a, const ObjectPoolKey& b){
            return !operator==(a, b);
        }

        friend std::ostream& operator<<(std::ostream& stream, const ObjectPoolKey& key){
            stream << "ObjectPoolKey(" << key.GetTag() << ", " << key.GetHash() << ")";
            return stream;
        }

        static inline int64_t
        GetSize(){
            return ObjectTag::GetSize()
                 + Hash::GetSize();
        }
    };

    class ObjectPool{
    public:
        enum State{
#define DEFINE_STATE(Name) k##Name,
            FOR_EACH_POOL_STATE(DEFINE_STATE)
#undef DEFINE_STATE
        };

        friend std::ostream& operator<<(std::ostream& stream, const State& state){
            switch(state){
#define DEFINE_TOSTRING(Name) \
                case State::k##Name: \
                    stream << #Name; \
                    return stream;
                FOR_EACH_POOL_STATE(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
                default:
                    stream << "Unknown";
                    return stream;
            }
        }

        enum Status{
#define DEFINE_STATUS(Name) k##Name,
            FOR_EACH_POOL_STATUS(DEFINE_STATUS)
#undef DEFINE_STATUS
        };

        friend std::ostream& operator<<(std::ostream& stream, const Status& status){
            switch(status){
#define DEFINE_TOSTRING(Name) \
                case Status::k##Name: \
                    stream << #Name; \
                    return stream;
                FOR_EACH_POOL_STATUS(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
                default:
                    stream << "Unknown";
                    return stream;
            }
        }
    private:
        ObjectPool() = delete;
        static void SetState(const State& state);
        static void SetStatus(const Status& status);
        static leveldb::Status IndexObject(const Hash& hash, const std::string& filename);
        static int64_t GetNumberOfObjectsByType(const ObjectTag::Type& type);
        static bool HasObjectByType(const Hash& hash, const ObjectTag::Type& type);
    public:
        ~ObjectPool() = delete;

        static State GetState();
        static Status GetStatus();
        static bool Initialize();
        static bool RemoveObject(const Hash& hash);
        static bool WaitForObject(const Hash& hash);
        static bool PutObject(const Hash& hash, const BlockPtr& val);
        static bool PutObject(const Hash& hash, const TransactionPtr& val);
        static bool PutObject(const Hash& hash, const UnclaimedTransactionPtr& val);
        static bool PutHashList(const User& user, const HashList& hashes);
        static bool GetBlocks(HashList& hashes);
        static bool GetTransactions(HashList& hashes);
        static bool GetUnclaimedTransactions(HashList& hashes);
        static bool GetUnclaimedTransactionsFor(HashList& hashes, const User& user);
        static bool HasHashList(const User& user);
        static bool VisitBlocks(ObjectPoolBlockVisitor* vis);
        static bool VisitTransactions(ObjectPoolTransactionVisitor* vis);
        static bool VisitUnclaimedTransactions(ObjectPoolUnclaimedTransactionVisitor* vis);
        static bool GetHashList(const User& user, HashList& hashes);
        static bool GetHashListAsJson(const User& user, JsonString& json);
        static BlockPtr GetBlock(const Hash& hash);
        static TransactionPtr GetTransaction(const Hash& hash);
        static UnclaimedTransactionPtr GetUnclaimedTransaction(const Hash& hash);
        static leveldb::Status Write(leveldb::WriteBatch* update);

        static bool HasObject(const Hash& hash);
        static inline bool HasBlock(const Hash& hash){
            return HasObjectByType(hash, ObjectTag::kBlock);
        }

        static inline bool HasTransaction(const Hash& hash){
            return HasObjectByType(hash, ObjectTag::kTransaction);
        }

        static inline bool HasUnclaimedTransaction(const Hash& hash){
            return HasObjectByType(hash, ObjectTag::kUnclaimedTransaction);
        }

        static int64_t GetNumberOfObjects();
        static inline int64_t GetNumberOfBlocks(){
            return GetNumberOfObjectsByType(ObjectTag::kBlock);
        }

        static inline int64_t GetNumberOfTransactions(){
            return GetNumberOfObjectsByType(ObjectTag::kTransaction);
        }

        static inline int64_t GetNumberOfUnclaimedTransactions(){
            return GetNumberOfObjectsByType(ObjectTag::kUnclaimedTransaction);
        }

        static inline bool GetUnclaimedTransactionsFor(HashList& hashes, const std::string& user){
            return GetUnclaimedTransactionsFor(hashes, User(user));
        }

        static inline bool GetHashListFor(const std::string& user, HashList& hashes){
            return GetHashList(User(user), hashes);
        }

        static inline bool PutHashList(const std::string& user, const HashList& hashes){
            return PutHashList(User(user), hashes);
        }

#define DEFINE_CHECK(Name) \
        static inline bool Is##Name(){ return GetState() == ObjectPool::k##Name; }
        FOR_EACH_POOL_STATE(DEFINE_CHECK)
#undef DEFINE_CHECK

#define DEFINE_CHECK(Name) \
        static inline bool Is##Name(){ return GetStatus() == ObjectPool::k##Name; }
        FOR_EACH_POOL_STATUS(DEFINE_CHECK)
#undef DEFINE_CHECK
    };
}

#endif //TOKEN_POOL_H