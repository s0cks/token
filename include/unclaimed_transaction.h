#ifndef TOKEN_UNCLAIMED_TRANSACTION_H
#define TOKEN_UNCLAIMED_TRANSACTION_H

#include "buffer.h"
#include "user.h"
#include "object.h"
#include "product.h"

namespace Token{
    class UnclaimedTransaction;

    typedef std::shared_ptr<UnclaimedTransaction> UnclaimedTransactionPtr;

    class Output;
    class Transaction;
    class UnclaimedTransaction : public BinaryObject{
        friend class UnclaimedTransactionMessage;
    private:
        Hash hash_;
        int32_t index_;
        User user_;
        Product product_;


    public:
        UnclaimedTransaction(const Hash& hash, int32_t index, const User& user, const Product& product):
            BinaryObject(Type::kUnclaimedTransactionType),
            hash_(hash),
            index_(index),
            user_(user),
            product_(product){}
        UnclaimedTransaction(const Hash& hash, int32_t index, const std::string& user, const std::string& product):
            UnclaimedTransaction(hash, index, User(user), Product(product)){}
        UnclaimedTransaction(Buffer* buff):
            UnclaimedTransaction(buff->GetHash(), buff->GetInt(), buff->GetUser(), buff->GetProduct()){}
        ~UnclaimedTransaction(){}

        Hash GetTransaction() const{
            return hash_;
        }

        int32_t GetIndex() const{
            return index_;
        }

        User GetUser() const{
            return user_;
        }

        Product GetProduct() const{
            return product_;
        }

        int64_t GetBufferSize() const{
            int64_t size = 0;
            size += Hash::kSize;
            size += sizeof(int32_t);
            size += User::kSize;
            size += Product::kSize;
            return size;
        }

        bool Encode(Buffer* buff) const{
            buff->PutHash(hash_);
            buff->PutInt(index_);
            buff->PutUser(user_);
            buff->PutProduct(product_);
            return true;
        }

        std::string ToString() const;

        static UnclaimedTransactionPtr NewInstance(std::fstream& fd, size_t size);
        static inline UnclaimedTransactionPtr NewInstance(const std::string& filename){
            std::fstream fd(filename, std::ios::in|std::ios::binary);
            return NewInstance(fd, GetFilesize(filename));
        }
    };

#define FOR_EACH_UTXOPOOL_STATE(V) \
    V(Uninitialized)               \
    V(Initializing)                 \
    V(Initialized)

#define FOR_EACH_UTXOPOOL_STATUS(V) \
    V(Ok)                           \
    V(Warning)                      \
    V(Error)

    class UnclaimedTransactionPoolVisitor;
    class UnclaimedTransactionPool{
    public:
        enum State{
#define DEFINE_STATE(Name) k##Name,
            FOR_EACH_UTXOPOOL_STATE(DEFINE_STATE)
#undef DEFINE_STATE
        };

        friend std::ostream& operator<<(std::ostream& stream, const State& state){
            switch(state){
#define DEFINE_TOSTRING(Name) \
                case State::k##Name: \
                    stream << #Name; \
                    return stream;
                FOR_EACH_UTXOPOOL_STATE(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
            }
        }

        enum Status{
#define DEFINE_STATUS(Name) k##Name,
            FOR_EACH_UTXOPOOL_STATUS(DEFINE_STATUS)
#undef DEFINE_STATUS
        };

        friend std::ostream& operator<<(std::ostream& stream, const Status& status){
            switch(status){
#define DEFINE_TOSTRING(Name) \
                case Status::k##Name: \
                    stream << #Name; \
                    return stream;
                FOR_EACH_UTXOPOOL_STATUS(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
            }
        }
    private:
        UnclaimedTransactionPool() = delete;
        static void SetState(State state);
        static void SetStatus(Status status);
    public:
        ~UnclaimedTransactionPool() = delete;

        static State GetState();
        static Status GetStatus();
        static std::string GetStatusMessage();
        static int64_t GetNumberOfUnclaimedTransactions();
        static bool Print();
        static bool Initialize();
        static bool Accept(UnclaimedTransactionPoolVisitor* vis);
        static bool RemoveUnclaimedTransaction(const Hash& hash);
        static bool PutUnclaimedTransaction(const Hash& hash, const UnclaimedTransactionPtr& utxo);
        static bool HasUnclaimedTransaction(const Hash& hash);
        static bool GetUnclaimedTransactions(std::vector<Hash>& utxos);
        static bool GetUnclaimedTransactions(const std::string& user, std::vector<Hash>& utxos);
        static UnclaimedTransactionPtr GetUnclaimedTransaction(const Hash& hash);
        static UnclaimedTransactionPtr GetUnclaimedTransaction(const Hash& tx_hash, int32_t tx_index);

#define DEFINE_STATE_CHECK(Name) \
        static inline bool Is##Name(){ return GetState() == UnclaimedTransactionPool::k##Name; }
        FOR_EACH_UTXOPOOL_STATE(DEFINE_STATE_CHECK)
#undef DEFINE_STATE_CHECK

#define DEFINE_STATUS_CHECK(Name) \
        static inline bool Is##Name(){ return GetStatus() == UnclaimedTransactionPool::k##Name; }
        FOR_EACH_UTXOPOOL_STATUS(DEFINE_STATUS_CHECK)
#undef DEFINE_STATUS_CHECK
    };

    class UnclaimedTransactionPoolVisitor{
    protected:
        UnclaimedTransactionPoolVisitor() = default;
    public:
        virtual ~UnclaimedTransactionPoolVisitor() = default;

        virtual bool VisitStart() { return true; }
        virtual bool Visit(const UnclaimedTransactionPtr& utxo) = 0;
        virtual bool VisitEnd() { return true; };
    };
}

#endif //TOKEN_UNCLAIMED_TRANSACTION_H