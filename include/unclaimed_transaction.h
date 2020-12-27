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
    };

    class UnclaimedTransactionFileWriter : BinaryFileWriter{
    public:
        UnclaimedTransactionFileWriter(const std::string& filename): BinaryFileWriter(filename){}
        UnclaimedTransactionFileWriter(BinaryFileWriter* parent): BinaryFileWriter(parent){}
        ~UnclaimedTransactionFileWriter() = default;

        bool Write(const UnclaimedTransactionPtr& utxo){
            WriteObjectTag(ObjectTag::kUnclaimedTransaction);

            WriteHash(utxo->GetTransaction());
            WriteInt(utxo->GetIndex());
            WriteUser(utxo->GetUser());
            WriteProduct(utxo->GetProduct());
            return true;
        }
    };

    class UnclaimedTransactionFileReader : BinaryFileReader{
    private:
        ObjectTagVerifier tag_verifier_;

        inline ObjectTagVerifier&
        GetTagVerifier(){
            return tag_verifier_;
        }
    public:
        UnclaimedTransactionFileReader(const std::string& filename):
            BinaryFileReader(filename),
            tag_verifier_(this, ObjectTag::kUnclaimedTransaction){}
        UnclaimedTransactionFileReader(BinaryFileReader* parent):
            BinaryFileReader(parent),
            tag_verifier_(this, ObjectTag::kUnclaimedTransaction){}
        ~UnclaimedTransactionFileReader() = default;

        UnclaimedTransactionPtr Read(){
            if(!GetTagVerifier().IsValid()){
                LOG(WARNING) << "couldn't read unclaimed transaction from: " << GetFilename();
                return UnclaimedTransactionPtr(nullptr);
            }

            Hash tx_hash = ReadHash();
            int32_t tx_index = ReadInt();
            User user = ReadUser();
            Product product = ReadProduct();
            return std::make_shared<UnclaimedTransaction>(tx_hash, tx_index, user, product);
        }
    };
}

#endif //TOKEN_UNCLAIMED_TRANSACTION_H