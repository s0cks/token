#ifndef TOKEN_OBJECT_H
#define TOKEN_OBJECT_H

#include <map>
#include <vector>
#include <string>

#include "common.h"
#include "uint256_t.h"
#include "merkle.h"

namespace Token{
    class UnclaimedTransaction;
    class Transaction;
    class Block;

    class BinaryObject{
    protected:
        virtual bool GetBytes(CryptoPP::SecByteBlock& bytes) const = 0;
    public:
        virtual ~BinaryObject() = default;

        uint256_t GetHash() const{
            CryptoPP::SHA256 func;
            CryptoPP::SecByteBlock bytes;
            if(!GetBytes(bytes)) return uint256_t();
            CryptoPP::SecByteBlock hash(CryptoPP::SHA256::DIGESTSIZE);
            CryptoPP::ArraySource source(bytes.data(), bytes.size(), true, new CryptoPP::HashFilter(func, new CryptoPP::ArraySink(hash.data(), hash.size())));
            return uint256_t(hash.data());
        }
    };

    class Input : public BinaryObject{
    private:
        uint256_t previous_hash_;
        uint32_t index_;

        friend class Transaction;
    protected:
        bool GetBytes(CryptoPP::SecByteBlock& bytes) const;
    public:
        typedef Proto::BlockChain::Input RawType;

        Input():
                previous_hash_(),
                index_(){}
        Input(const Input& other):
                previous_hash_(other.previous_hash_),
                index_(other.index_){}
        Input(const UnclaimedTransaction& utxo);
        Input(const RawType& raw):
                previous_hash_(HashFromHexString(raw.previous_hash())),
                index_(raw.index()){}
        ~Input(){}

        uint32_t GetOutputIndex() const{
            return index_;
        }

        uint256_t GetTransactionHash() const{
            return previous_hash_;
        }

        bool GetTransaction(Transaction* result) const;
        bool GetUnclaimedTransaction(UnclaimedTransaction* result) const;

        Input& operator=(const Input& other){
            index_ = other.index_;
            previous_hash_ = other.previous_hash_;
            return (*this);
        }

        friend bool operator==(const Input& lhs, const Input& rhs){
            return lhs.GetHash() == rhs.GetHash();
        }

        friend bool operator!=(const Input& lhs, const Input& rhs){
            return !operator==(lhs, rhs);
        }

        friend RawType& operator<<(RawType& stream, const Input& input){
            stream.set_previous_hash(HexString(input.GetTransactionHash()));
            stream.set_index(input.GetOutputIndex());
            return stream;
        }
    };

    class Output : public BinaryObject{
    private:
        std::string token_;
        std::string user_;

        friend class Transaction;
    protected:
        bool GetBytes(CryptoPP::SecByteBlock& bytes) const;
    public:
        typedef Proto::BlockChain::Output RawType;

        Output():
            user_(),
            token_(){}
        Output(const std::string& user, const std::string& token):
            user_(user),
            token_(token){}
        Output(const RawType& raw):
            Output(raw.user(), raw.token()){}
        Output(const Output& other):
            Output(other.user_, other.token_){}
        ~Output(){}

        std::string GetUser() const{
            return user_;
        }

        std::string GetToken() const{
            return token_;
        }

        Output& operator=(const Output& other){
            user_ = other.user_;
            token_ = other.token_;
            return (*this);
        }

        friend bool operator==(const Output& lhs, const Output& rhs){
            return lhs.GetHash() == rhs.GetHash();
        }

        friend bool operator!=(const Output& lhs, const Output& rhs){
            return !operator==(lhs, rhs);
        }

        friend RawType& operator<<(RawType& stream, const Output& output){
            stream.set_user(output.user_);
            stream.set_token(output.token_);
            return stream;
        }
    };

    class TransactionVisitor;
    class Transaction : public BinaryObject{
    public:
        typedef Proto::BlockChain::Transaction RawType;
        typedef std::vector<Input> InputList;
        typedef std::vector<Output> OutputList;
    private:
        uint32_t timestamp_;
        uint32_t index_;
        InputList inputs_;
        OutputList outputs_;
        std::string signature_;

        friend class Block;
        friend class TransactionPool;
        friend class TransactionVerifier;
    protected:
        bool GetBytes(CryptoPP::SecByteBlock& bytes) const;
    public:
        Transaction():
            timestamp_(0),
            index_(0),
            inputs_(),
            outputs_(),
            signature_(""),
            BinaryObject(){}
        Transaction(uint64_t index, uint64_t timestamp=GetCurrentTime()):
            timestamp_(timestamp),
            index_(index),
            inputs_(),
            outputs_(),
            signature_(),
            BinaryObject(){}
        Transaction(const RawType& raw):
            timestamp_(raw.timestamp()),
            index_(raw.index()),
            signature_(raw.signature()),
            inputs_(),
            outputs_(),
            BinaryObject(){
            for(auto& it : raw.inputs()) inputs_.push_back(Input(it));
            for(auto& it : raw.outputs()) outputs_.push_back(Output(it));
        }
        Transaction(const Transaction& other):
            timestamp_(other.timestamp_),
            signature_(other.signature_),
            index_(other.index_),
            inputs_(other.inputs_),
            outputs_(other.outputs_){}
        ~Transaction(){}

        std::string GetSignature() const{
            return signature_;
        }

        bool GetInput(uint32_t idx, Input* result) const{
            if(idx < 0 || idx > inputs_.size()) return false;
            (*result) = inputs_[idx];
            return true;
        }

        bool GetOutput(uint32_t idx, Output* result) const{
            if(idx < 0 || idx > outputs_.size()) return false;
            (*result) = outputs_[idx];
            return true;
        }

        uint32_t GetNumberOfInputs() const{
            return inputs_.size();
        }

        uint32_t GetNumberOfOutputs() const{
            return outputs_.size();
        }

        uint32_t GetIndex() const{
            return index_;
        }

        uint32_t GetTimestamp() const{
            return timestamp_;
        }

        bool IsCoinbase() const{
            return GetIndex() == 0;
        }

        bool IsSigned() const{
            return !signature_.empty();
        }

        bool Sign();
        bool Accept(TransactionVisitor* visitor);

        InputList::const_iterator inputs_begin() const{
            return inputs_.begin();
        }

        InputList::const_iterator inputs_end() const{
            return inputs_.end();
        }

        OutputList::const_iterator outputs_begin() const{
            return outputs_.begin();
        }

        OutputList::const_iterator outputs_end() const{
            return outputs_.end();
        }

        Transaction& operator=(const Transaction& other){
            index_ = other.index_;
            timestamp_ = other.timestamp_;
            signature_ = other.signature_;
            inputs_ = std::vector<Input>(other.inputs_);
            outputs_ = std::vector<Output>(other.outputs_);
            return (*this);
        }

        friend bool operator==(const Transaction& lhs, const Transaction& rhs){
            return lhs.GetHash() == rhs.GetHash();
        }

        friend bool operator!=(const Transaction& lhs, const Transaction& rhs){
            return !operator==(lhs, rhs);
        }

        friend bool operator<(const Transaction& lhs, const Transaction& rhs){
            return lhs.GetTimestamp() < rhs.GetTimestamp();
        }

        friend Transaction& operator<<(Transaction& stream, const Output& output){
            stream.outputs_.push_back(output);
            return stream;
        }

        friend Transaction& operator<<(Transaction& stream, const Input& input){
            stream.inputs_.push_back(input);
            return stream;
        }

        friend RawType& operator<<(RawType& stream, const Transaction& tx){
            stream.set_index(tx.GetIndex());
            stream.set_timestamp(tx.GetTimestamp());
            if(tx.IsSigned()) stream.set_signature(tx.GetSignature());
            uintptr_t idx;
            for(idx = 0; idx < tx.GetNumberOfInputs(); idx++){
                Input::RawType* raw = stream.add_inputs();
                (*raw) << tx.inputs_[idx];
            }
            for(idx = 0; idx < tx.GetNumberOfOutputs(); idx++){
                Output::RawType* raw = stream.add_outputs();
                (*raw) << tx.outputs_[idx];
            }
            return stream;
        }
    };

    class UnclaimedTransaction : public BinaryObject{
    private:
        uint256_t hash_; // Transaction Hash
        uint32_t index_; // Output Index
    protected:
        bool GetBytes(CryptoPP::SecByteBlock& bytes) const;
    public:
        typedef Proto::BlockChain::UnclaimedTransaction RawType;

        UnclaimedTransaction():
            hash_(),
            index_(){}
        UnclaimedTransaction(const Transaction& tx, uint32_t index):
            hash_(tx.GetHash()),
            index_(index){}
        UnclaimedTransaction(const RawType& raw):
            hash_(HashFromHexString(raw.tx_hash())),
            index_(raw.tx_index()){}
        UnclaimedTransaction(const UnclaimedTransaction& other):
            hash_(other.hash_),
            index_(other.index_){}
        ~UnclaimedTransaction(){}

        uint256_t GetTransaction() const{
            return hash_;
        }

        uint32_t GetIndex() const{
            return index_;
        }

        bool GetTransaction(Transaction* result) const;
        bool GetOutput(Output* result) const;

        friend bool operator==(const UnclaimedTransaction& lhs, const UnclaimedTransaction& rhs){
            return lhs.GetHash() == rhs.GetHash();
        }

        friend bool operator!=(const UnclaimedTransaction& lhs, const UnclaimedTransaction& rhs){
            return !operator==(lhs, rhs);
        }

        friend RawType& operator<<(RawType& stream, const UnclaimedTransaction& utxo){
            stream.set_tx_hash(HexString(utxo.GetTransaction()));
            stream.set_tx_index(utxo.GetIndex());
            return stream;
        }

        UnclaimedTransaction& operator=(const UnclaimedTransaction& other){
            hash_ = other.hash_;
            index_ = other.index_;
            return (*this);
        }
    };

    class TransactionVisitor{
    public:
        virtual ~TransactionVisitor() = default;

        virtual bool VisitStart(){ return true; }
        virtual bool VisitInputsStart(){ return true; }
        virtual bool VisitInput(const Input& input) = 0;
        virtual bool VisitInputsEnd(){ return true; }
        virtual bool VisitOutputsStart(){ return true; }
        virtual bool VisitOutput(const Output& output) = 0;
        virtual bool VisitOutputsEnd(){ return true; }
        virtual bool VisitEnd(){ return true; }
    };

    class BlockHeader{
    private:
        uint32_t timestamp_;
        uint32_t height_;
        uint256_t previous_hash_;
        uint256_t hash_;
        uint256_t merkle_root_;
    public:
        typedef Proto::BlockChain::BlockHeader RawType;

        BlockHeader():
            timestamp_(0),
            height_(0),
            previous_hash_(),
            hash_(),
            merkle_root_(){}
        BlockHeader(const Block& block);
        BlockHeader(const RawType& raw):
            timestamp_(raw.timestamp()),
            height_(raw.height()),
            previous_hash_(HashFromHexString(raw.previous_hash())),
            hash_(HashFromHexString(raw.hash())),
            merkle_root_(HashFromHexString(raw.merkle_root())){}
        ~BlockHeader(){}

        uint32_t GetTimestamp() const{
            return timestamp_;
        }

        uint32_t GetHeight() const{
            return height_;
        }

        uint256_t GetPreviousHash() const{
            return previous_hash_;
        }

        uint256_t GetHash() const{
            return hash_;
        }

        uint256_t GetMerkleRoot() const{
            return merkle_root_;
        }

        Block* GetData() const;

        friend bool operator==(const BlockHeader& a, const BlockHeader& b){
            return a.GetHash() == b.GetHash();
        }

        friend bool operator!=(const BlockHeader& a, const BlockHeader& b){
            return !operator==(a, b);
        }

        BlockHeader& operator=(const BlockHeader& other){
            timestamp_ = other.timestamp_;
            height_ = other.height_;
            previous_hash_ = other.previous_hash_;
            hash_ = other.hash_;
            merkle_root_ = other.merkle_root_;
            return (*this);
        }

        friend Proto::BlockChain::BlockHeader& operator<<(Proto::BlockChain::BlockHeader& stream, const BlockHeader& block){
            stream.set_timestamp(block.GetTimestamp());
            stream.set_height(block.GetHeight());
            stream.set_merkle_root(HexString(block.GetMerkleRoot()));
            stream.set_hash(HexString(block.GetHash()));
            stream.set_previous_hash(HexString(block.GetPreviousHash()));
            return stream;
        }
    };

    class BlockVisitor;
    class Block : public BinaryObject{
    private:
        typedef std::map<uint256_t, Transaction> TransactionMap;
        typedef std::pair<uint256_t, Transaction> TransactionMapPair;
        typedef std::vector<Transaction> TransactionList;

        uint32_t timestamp_;
        uint32_t height_;
        uint256_t previous_hash_;
        TransactionList tx_list_;
        TransactionMap tx_map_;

        bool GetBytes(CryptoPP::SecByteBlock& bytes) const;

        friend class BlockChain;
    public:
        typedef Proto::BlockChain::Block RawType;

        Block():
            timestamp_(0),
            height_(0),
            previous_hash_(),
            tx_list_(),
            tx_map_(){}
        Block(uint32_t height, const uint256_t& previous_hash, const std::vector<Transaction>& transactions, uint32_t timestamp=GetCurrentTime()):
            timestamp_(timestamp),
            height_(height),
            previous_hash_(previous_hash),
            tx_list_(transactions),
            tx_map_(){
            for(auto& it : transactions) tx_map_.insert(TransactionMapPair(it.GetHash(), it));
        }
        Block(const BlockHeader& parent, const std::vector<Transaction>& transactions, uint32_t timestamp=GetCurrentTime()):
            timestamp_(timestamp),
            height_(parent.GetHeight() + 1),
            previous_hash_(parent.GetHash()),
            tx_list_(transactions),
            tx_map_(){
            for(auto& it : transactions) tx_map_.insert(TransactionMapPair(it.GetHash(), it));
        }
        Block(Block* parent, const std::vector<Transaction>& transactions, uint32_t timestamp=GetCurrentTime()):
            timestamp_(timestamp),
            height_(parent->GetHeight() + 1),
            previous_hash_(parent->GetHash()),
            tx_list_(transactions),
            tx_map_(){
            for(auto& it : transactions) tx_map_.insert(TransactionMapPair(it.GetHash(), it));
        }
        Block(const Proto::BlockChain::Block& raw):
            timestamp_(raw.timestamp()),
            height_(raw.height()),
            previous_hash_(HashFromHexString(raw.previous_hash())),
            tx_list_(),
            tx_map_(){
            for(auto& it : raw.transactions()){
                Transaction tx(it);
                tx_list_.push_back(it);
                tx_map_.insert(TransactionMapPair(tx.GetHash(), tx));
            }
        }
        Block(const Block& other):
            timestamp_(other.timestamp_),
            height_(other.height_),
            previous_hash_(other.previous_hash_),
            tx_list_(other.tx_list_),
            tx_map_(other.tx_map_){}
        ~Block(){}

        MerkleTree* GetMerkleTree() const;
        uint256_t GetMerkleRoot() const;
        uint256_t GetPreviousHash() const{
            return previous_hash_;
        }

        uint32_t GetHeight() const{
            return height_;
        }

        uint32_t GetNumberOfTransactions() const{
            return tx_list_.size();
        }

        uint32_t GetTimestamp() const{
            return timestamp_;
        }

        bool GetTransaction(uint64_t idx, Transaction* result) const{
            if(idx < 0 || idx > GetNumberOfTransactions()) return false;
            (*result) = tx_list_[idx];
            return true;
        }

        bool GetTransaction(const uint256_t& hash, Transaction* result) const{
            auto pos = tx_map_.find(hash);
            if(pos == tx_map_.end()) return false;
            (*result) = pos->second;
            return true;
        }

        bool IsGenesis(){
            return GetHeight() == 0;
        }

        bool Contains(const uint256_t& hash) const;
        bool Accept(BlockVisitor* vis) const;

        TransactionList::const_iterator begin() const{
            return tx_list_.begin();
        }

        TransactionList::const_iterator end() const{
            return tx_list_.end();
        }

        friend bool operator==(const Block& lhs, const Block& rhs){
            return const_cast<Block&>(lhs).GetHash() == const_cast<Block&>(rhs).GetHash();
        }

        friend bool operator!=(const Block& lhs, const Block& rhs){
            return !operator==(lhs, rhs);
        }

        friend RawType& operator<<(RawType& stream, const Block& block){
            stream.set_timestamp(block.GetTimestamp());
            stream.set_height(block.GetHeight());
            stream.set_merkle_root(HexString(block.GetMerkleRoot()));
            stream.set_previous_hash(HexString(block.GetPreviousHash()));
            for(size_t idx = 0; idx < block.GetNumberOfTransactions(); idx++){
                Proto::BlockChain::Transaction* raw = stream.add_transactions();
                (*raw) << block.tx_list_[idx];
            }
            return stream;
        }

        Block& operator=(const Block& other){
            timestamp_ = other.timestamp_;
            height_ = other.height_;
            previous_hash_ = other.previous_hash_;
            tx_list_ = other.tx_list_;
            tx_map_ = other.tx_map_;
            return (*this);
        }
    };

    class BlockVisitor{
    public:
        BlockVisitor(){}
        virtual ~BlockVisitor(){}

        virtual bool VisitStart(){ return true; }
        virtual bool Visit(const Transaction& tx) = 0;
        virtual bool VisitEnd(){ return true; }
    };
}

#endif //TOKEN_OBJECT_H