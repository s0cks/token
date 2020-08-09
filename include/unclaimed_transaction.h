#ifndef TOKEN_UNCLAIMED_TRANSACTION_H
#define TOKEN_UNCLAIMED_TRANSACTION_H

#include "common.h"
#include "object.h"

namespace Token{
    class Output;
    class Transaction;
    class UnclaimedTransaction : public Object{
    private:
        uint256_t hash_;
        uint32_t index_;
        std::string user_;

        UnclaimedTransaction(const uint256_t& hash, uint32_t idx, const std::string& user):
            hash_(hash),
            user_(user),
            index_(idx){}
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

        bool Encode(uint8_t* bytes) const;
        size_t GetBufferSize() const;
        std::string ToString() const;

        static Handle<UnclaimedTransaction> NewInstance(const uint256_t &hash, uint32_t index, const std::string& user){
            return new UnclaimedTransaction(hash, index, user);
        }

        static Handle<UnclaimedTransaction> NewInstance(uint8_t* bytes);
        static Handle<UnclaimedTransaction> NewInstance(std::fstream& fd);

        static inline Handle<UnclaimedTransaction> NewInstance(const std::string& filename){
            std::fstream fd(filename, std::ios::in|std::ios::binary);
            return NewInstance(fd);
        }
    };
}

#endif //TOKEN_UNCLAIMED_TRANSACTION_H