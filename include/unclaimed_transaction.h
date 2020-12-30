#ifndef TOKEN_UNCLAIMED_TRANSACTION_H
#define TOKEN_UNCLAIMED_TRANSACTION_H

#include "utils/buffer.h"
#include "object.h"
#include "utils/file_writer.h"
#include "utils/file_reader.h"

namespace Token{
    class UnclaimedTransaction;
    typedef std::shared_ptr<UnclaimedTransaction> UnclaimedTransactionPtr;

    class Output;
    class Transaction;
    class UnclaimedTransaction : public BinaryObject{
        friend class UnclaimedTransactionMessage;
    public:
        enum UnclaimedTransactionLayout{
            // TransactionHash
            kTxHashOffset = 0,
            kTxHashSize = 256/8,

            // OutputIndex
            kIndexOffset = kTxHashOffset+kTxHashSize,
            kIndexSize = sizeof(int64_t),

            // User
            kUserOffset = kIndexOffset+kIndexSize,
            kUserSize = User::kSize,

            // Product
            kProductOffset = kUserOffset+kUserSize,
            kProductSize = Product::kSize,
            kTotalSize = kProductOffset+kProductSize,
        };
    private:
        Hash hash_;
        int32_t index_;
        User user_;
        Product product_;
    protected:
        int64_t GetBufferSize() const{
            int64_t size = 0;
            size += Hash::GetSize();
            size += sizeof(int32_t);
            size += User::GetSize();
            size += Product::GetSize();
            return size;
        }

        bool Write(Buffer* buff) const{
            buff->PutHash(hash_);
            buff->PutInt(index_);
            buff->PutUser(user_);
            buff->PutProduct(product_);
            return true;
        }
    public:
        UnclaimedTransaction(uint8_t* bytes, const int64_t& size):
            BinaryObject(),
            hash_(&bytes[kTxHashOffset], kTxHashSize),
            index_(),
            user_(&bytes[kUserOffset], kUserSize),
            product_(&bytes[kProductOffset], kProductSize){
            memcpy(&index_, &bytes[kIndexOffset], kIndexSize);
        }
        UnclaimedTransaction(const Hash& hash, int32_t index, const User& user, const Product& product):
            BinaryObject(),
            hash_(hash),
            index_(index),
            user_(user),
            product_(product){}
        UnclaimedTransaction(const Hash& hash, int32_t index, const std::string& user, const std::string& product):
            UnclaimedTransaction(hash, index, User(user), Product(product)){}
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

        std::string ToString() const;

        bool Encode(uint8_t* bytes, const int64_t& size) const{
            if(size < kTotalSize)
                return false;
            memcpy(&bytes[kTxHashOffset], hash_.data(), kTxHashSize);
            memcpy(&bytes[kIndexOffset], &index_, kIndexSize);
            memcpy(&bytes[kUserOffset], user_.data(), kUserSize);
            memcpy(&bytes[kProductOffset], product_.data(), kProductSize);
            return true;
        }

        bool Encode(leveldb::Slice* slice) const{
            if(!slice)
                return false;
            uint8_t bytes[kTotalSize];
            Encode(bytes, kTotalSize);
            (*slice) = leveldb::Slice((char*)bytes, kTotalSize);
            return true;
        }

        static UnclaimedTransactionPtr NewInstance(Buffer* buff){
            Hash hash = buff->GetHash();
            int32_t idx = buff->GetInt();
            User usr = buff->GetUser();
            Product product = buff->GetProduct();
            return std::make_shared<UnclaimedTransaction>(hash, idx, usr, product);
        }

        static UnclaimedTransactionPtr NewInstance(std::fstream& fd, size_t size);
        static inline UnclaimedTransactionPtr NewInstance(const std::string& filename){
            std::fstream fd(filename, std::ios::in|std::ios::binary);
            return NewInstance(fd, GetFilesize(filename));
        }

        static inline int64_t
        GetSize(){
            int64_t size = 0;
            size += Hash::GetSize();
            size += sizeof(int64_t);
            size += User::GetSize();
            size += Product::GetSize();
            return size;
        }
    };
}

#endif //TOKEN_UNCLAIMED_TRANSACTION_H