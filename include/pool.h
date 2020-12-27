#ifndef TOKEN_POOL_H
#define TOKEN_POOL_H

#include <leveldb/db.h>
#include "hash.h"
#include "block.h"
#include "transaction.h"
#include "unclaimed_transaction.h"

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

        template<class ObjectType, class ObjectWriter>
        static inline bool
        WriteObject(const std::shared_ptr<ObjectType>& obj, const std::string& filename){
            ObjectWriter writer(filename);
            return writer.Write(obj);
        }
    public:
        ~ObjectPool() = delete;

        static State GetState();
        static Status GetStatus();
        static bool Initialize();
        static bool HasObject(const Hash& hash);
        static bool RemoveObject(const Hash& hash);
        static bool WaitForObject(const Hash& hash);
        static bool PutObject(const Hash& hash, const BlockPtr& val);
        static bool PutObject(const Hash& hash, const TransactionPtr& val);
        static bool PutObject(const Hash& hash, const UnclaimedTransactionPtr& val);
        static bool GetBlocks(HashList& hashes);
        static bool GetTransactions(HashList& hashes);
        static bool GetUnclaimedTransactions(HashList& hashes);
        static bool VisitBlocks(ObjectPoolBlockVisitor* vis);
        static bool VisitTransactions(ObjectPoolTransactionVisitor* vis);
        static bool VisitUnclaimedTransactions(ObjectPoolUnclaimedTransactionVisitor* vis);
        static BlockPtr GetBlock(const Hash& hash);
        static TransactionPtr GetTransaction(const Hash& hash);
        static UnclaimedTransactionPtr GetUnclaimedTransaction(const Hash& hash);
        static int64_t GetNumberOfObjects();
        static int64_t GetNumberOfBlocks();
        static int64_t GetNumberOfTransactions();
        static int64_t GetNumberOfUnclaimedTransactions();

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