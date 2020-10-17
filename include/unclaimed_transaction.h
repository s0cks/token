#ifndef TOKEN_UNCLAIMED_TRANSACTION_H
#define TOKEN_UNCLAIMED_TRANSACTION_H

#include "common.h"

#include "user.h"
#include "object.h"

namespace Token{
    class Output;
    class Transaction;
    class UnclaimedTransaction : public BinaryObject{
    private:
        Hash hash_;
        uint32_t index_; //TODO: convert to intptr_t?
        User user_;

        UnclaimedTransaction(const Hash& hash, uint32_t idx, const User& user):
            hash_(hash),
            index_(idx),
            user_(user){}

        bool Write(ByteBuffer* bytes) const{
            bytes->PutHash(hash_);
            bytes->PutInt(index_);
            user_.Encode(bytes);
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

        std::string ToString() const;

        static Handle<UnclaimedTransaction> NewInstance(ByteBuffer* bytes);
        static Handle<UnclaimedTransaction> NewInstance(std::fstream& fd, size_t size);
        static Handle<UnclaimedTransaction> NewInstance(const Hash &hash, uint32_t index, const std::string& user){
            return new UnclaimedTransaction(hash, index, User(user));
        }

        static Handle<UnclaimedTransaction> NewInstance(const Hash& hash, uint32_t index, const User& user){
            return new UnclaimedTransaction(hash, index, user);
        }

        static inline Handle<UnclaimedTransaction> NewInstance(const std::string& filename){
            std::fstream fd(filename, std::ios::in|std::ios::binary);
            return NewInstance(fd, GetFilesize(filename));
        }
    };
}

#endif //TOKEN_UNCLAIMED_TRANSACTION_H