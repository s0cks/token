#ifndef TOKEN_UNCLAIMED_TRANSACTION_H
#define TOKEN_UNCLAIMED_TRANSACTION_H

#include "common.h"
#include "user.h"
#include "object.h"
#include "product.h"

namespace Token{
    class Output;
    class Transaction;
    class UnclaimedTransaction : public BinaryObject{
        friend class UnclaimedTransactionMessage;
    private:
        Hash hash_;
        uint32_t index_; //TODO: convert to intptr_t?
        User user_;
        Product product_;

        UnclaimedTransaction(const Hash& hash, uint32_t idx, const User& user, const Product& product):
            hash_(hash),
            index_(idx),
            user_(user),
            product_(product){}
    protected:
        intptr_t GetMessageSize() const{
            intptr_t size = 0;
            size += Hash::kSize;
            size += sizeof(uint32_t);
            size += User::kSize;
            size += Product::kSize;
            return size;
        }

        bool Write(ByteBuffer* bytes) const{
            bytes->PutHash(hash_);
            bytes->PutInt(index_);
            user_.Encode(bytes);
            product_.Encode(bytes);
            return true;
        }
    public:
        ~UnclaimedTransaction(){}

        Hash GetTransaction() const{
            return hash_;
        }

        uint32_t GetIndex() const{
            return index_;
        }

        User GetUser() const{
            return user_;
        }

        Product GetProduct() const{
            return product_;
        }

        std::string ToString() const;

        static Handle<UnclaimedTransaction> NewInstance(ByteBuffer* bytes);
        static Handle<UnclaimedTransaction> NewInstance(std::fstream& fd, size_t size);
        static Handle<UnclaimedTransaction> NewInstance(const Hash &hash, uint32_t index, const std::string& user, const std::string& product){
            return new UnclaimedTransaction(hash, index, User(user), Product(product));
        }

        static Handle<UnclaimedTransaction> NewInstance(const Hash& hash, uint32_t index, const User& user, const Product& product){
            return new UnclaimedTransaction(hash, index, user, product);
        }

        static inline Handle<UnclaimedTransaction> NewInstance(const std::string& filename){
            std::fstream fd(filename, std::ios::in|std::ios::binary);
            return NewInstance(fd, GetFilesize(filename));
        }
    };
}

#endif //TOKEN_UNCLAIMED_TRANSACTION_H