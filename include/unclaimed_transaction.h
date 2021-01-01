#ifndef TOKEN_UNCLAIMED_TRANSACTION_H
#define TOKEN_UNCLAIMED_TRANSACTION_H

#include "object.h"
#include "utils/buffer.h"

namespace Token{
    class UnclaimedTransaction;
    typedef std::shared_ptr<UnclaimedTransaction> UnclaimedTransactionPtr;

    class Output;
    class Transaction;
    class UnclaimedTransaction : public BinaryObject{
        friend class UnclaimedTransactionMessage;
    public:
        static const int64_t kSize = Hash::kSize + sizeof(int64_t) + User::kSize + Product::kSize;
    private:
        Hash hash_;
        int64_t index_;
        User user_;
        Product product_;
    public:
        UnclaimedTransaction(const Hash& hash, int64_t index, const User& user, const Product& product):
            BinaryObject(),
            hash_(hash),
            index_(index),
            user_(user),
            product_(product){}
        UnclaimedTransaction(const Hash& hash, int32_t index, const std::string& user, const std::string& product):
            UnclaimedTransaction(hash, index, User(user), Product(product)){}
        UnclaimedTransaction(const BufferPtr& buffer):
            UnclaimedTransaction(buffer->GetHash(), buffer->GetLong(), buffer->GetUser(), buffer->GetProduct()){}
        ~UnclaimedTransaction(){}

        Hash GetTransaction() const{
            return hash_;
        }

        int64_t GetIndex() const{
            return index_;
        }

        User GetUser() const{
            return user_;
        }

        Product GetProduct() const{
            return product_;
        }

        int64_t GetBufferSize() const{
            return UnclaimedTransaction::kSize;
        }

        bool Write(const BufferPtr& buff) const{
            buff->PutHash(hash_);
            buff->PutLong(index_);
            buff->PutUser(user_);
            buff->PutProduct(product_);
            return true;
        }

        std::string ToString() const{
            std::stringstream stream;
            stream << "UnclaimedTransaction(" << hash_ << "[" << index_ << "])";
            return stream.str();
        }

        static UnclaimedTransactionPtr NewInstance(const BufferPtr& buff){
            return std::make_shared<UnclaimedTransaction>(buff);
        }
    };
}

#endif //TOKEN_UNCLAIMED_TRANSACTION_H