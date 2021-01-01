#ifndef TOKEN_TRANSACTION_H
#define TOKEN_TRANSACTION_H

#include <set>
#include "hash.h"
#include "object.h"
#include "utils/buffer.h"
#include "unclaimed_transaction.h"

namespace Token{
  typedef std::shared_ptr<Transaction> TransactionPtr;

  class Input : public BinaryObject{
    friend class Transaction;
   public:
    static const int64_t kSize = Hash::kSize + sizeof(int64_t) + User::kSize;
   private:
    Hash hash_;
    int64_t index_;
    User user_;//TODO: remove field
   public:
    Input(const Hash& tx_hash, int64_t index, const User& user):
      BinaryObject(),
      hash_(tx_hash),
      index_(index),
      user_(user){}
    Input(const Hash& tx_hash, int64_t index, const std::string& user):
      Input(tx_hash, index, User(user)){}
    Input(const BufferPtr& buff):
      Input(buff->GetHash(), buff->GetInt(), buff->GetUser()){}
    ~Input(){}

    int64_t GetOutputIndex() const{
      return index_;
    }

    Hash GetTransactionHash() const{
      return hash_;
    }

    User GetUser() const{
      return user_;
    }

    UnclaimedTransactionPtr GetUnclaimedTransaction() const; //TODO: remove

    int64_t GetBufferSize() const{
      return Input::kSize;
    }

    bool Write(const BufferPtr& buffer) const{
      buffer->PutHash(hash_);
      buffer->PutLong(index_);
      buffer->PutUser(user_);
      return true;
    }

    std::string ToString() const{
      std::stringstream stream;
      stream << "Input(" << GetTransactionHash() << "[" << GetOutputIndex() << "]" << ", " << GetUser() << ")";
      return stream.str();
    }

    void operator=(const Input& other){
      hash_ = other.hash_;
      user_ = other.user_;
      index_ = other.index_;
    }

    friend bool operator==(const Input& a, const Input& b){
      return a.hash_ == b.hash_
             && a.index_ == b.index_
             && a.user_ == b.user_;
    }

    friend bool operator!=(const Input& a, const Input& b){
      return a.hash_ != b.hash_
             && a.index_ != b.index_
             && a.user_ != b.user_;
    }

    friend bool operator<(const Input& a, const Input& b){
      if(a.hash_ == b.hash_)
        return a.index_ < b.index_;
      return a.hash_ < b.hash_;
    }
  };

  class Output : public BinaryObject{
    friend class Transaction;
   public:
    static const int64_t kSize = User::kSize + Product::kSize;
   private:
    User user_;
    Product product_;
   public:
    Output(const User& user, const Product& product):
      BinaryObject(),
      user_(user),
      product_(product){}
    Output(const std::string& user, const std::string& product):
      Output(User(user), Product(product)){}
    Output(const BufferPtr& buff):
      Output(buff->GetUser(), buff->GetProduct()){}
    ~Output(){}

    User GetUser() const{
      return user_;
    }

    Product GetProduct() const{
      return product_;
    }

    int64_t GetBufferSize() const{
      return Output::kSize;
    }

    bool Write(const BufferPtr& buff) const{
      buff->PutUser(user_);
      buff->PutProduct(product_);
      return true;
    }

    std::string ToString() const{
      std::stringstream stream;
      stream << "Output(" << GetUser() << "; " << GetProduct() << ")";
      return stream.str();
    }

    void operator=(const Output& other){
      user_ = other.user_;
      product_ = other.product_;
    }

    friend bool operator==(const Output& a, const Output& b){
      return a.user_ == b.user_
             && a.product_ == b.product_;
    }

    friend bool operator!=(const Output& a, const Output& b){
      return a.user_ != b.user_
             && a.product_ != b.product_;
    }

    friend bool operator<(const Output& a, const Output& b){
      if(a.user_ == b.user_)
        return a.product_ < b.product_;
      return a.user_ < b.user_;
    }
  };

  typedef std::vector<Input> InputList;
  typedef std::vector<Output> OutputList;

  class TransactionInputVisitor;
  class TransactionOutputVisitor;
  class Transaction : public BinaryObject{
    friend class Block;
    friend class TransactionMessage;
   public:
    struct DefaultComparator{
      bool operator()(const TransactionPtr& a, const TransactionPtr& b){
        if(a->timestamp_ < b->timestamp_){
          return -1;
        } else if(a->timestamp_ > b->timestamp_){
          return +1;
        }

        if(a->index_ < b->index_){
          return -1;
        } else if(a->index_ > b->index_){
          return +1;
        }
        return 0;
      }
    };

    struct TimestampComparator{
      bool operator()(const TransactionPtr& a, const TransactionPtr& b){
        return a->timestamp_ < b->timestamp_;
      }
    };

    struct IndexComparator{
      bool operator()(const TransactionPtr& a, const TransactionPtr& b){
        return a->index_ < b->index_;
      }
    };

    static const int64_t kMaxNumberOfInputs;
    static const int64_t kMaxNumberOfOutputs;
   private:
    Timestamp timestamp_;
    int64_t index_;
    InputList inputs_;
    OutputList outputs_;
    std::string signature_;
   public:
    Transaction(int64_t index,
                const InputList& inputs,
                const OutputList& outputs,
                Timestamp timestamp = GetCurrentTimestamp()):
      BinaryObject(),
      timestamp_(timestamp),
      index_(index),
      inputs_(inputs),
      outputs_(outputs),
      signature_(){}
    ~Transaction() = default;

    Timestamp GetTimestamp() const{
      return timestamp_;
    }

    int64_t GetIndex() const{
      return index_;
    }

    int64_t GetNumberOfInputs() const{
      return inputs_.size();
    }

    InputList& inputs(){
      return inputs_;
    }

    InputList inputs() const{
      return inputs_;
    }

    OutputList& outputs(){
      return outputs_;
    }

    OutputList outputs() const{
      return outputs_;
    }

    InputList::iterator inputs_begin(){
      return inputs_.begin();
    }

    InputList::const_iterator inputs_begin() const{
      return inputs_.begin();
    }

    InputList::iterator inputs_end(){
      return inputs_.end();
    }

    InputList::const_iterator inputs_end() const{
      return inputs_.end();
    }

    int64_t GetNumberOfOutputs() const{
      return outputs_.size();
    }

    OutputList::iterator outputs_begin(){
      return outputs_.begin();
    }

    OutputList::const_iterator outputs_begin() const{
      return outputs_.begin();
    }

    OutputList::iterator outputs_end(){
      return outputs_.end();
    }

    OutputList::const_iterator outputs_end() const{
      return outputs_.end();
    }

    std::string GetSignature() const{
      return signature_;
    }

    bool IsSigned() const{
      return !signature_.empty();
    }

    bool IsCoinbase() const{
      return GetIndex() == 0;
    }

    int64_t GetBufferSize() const{
      int64_t size = 0;
      size += sizeof(Timestamp); // timestamp_
      size += sizeof(int64_t); // index_
      size += sizeof(int64_t); // num_inputs_
      size += GetNumberOfInputs() * Input::kSize; // inputs_
      size += sizeof(int64_t); // num_outputs
      size += GetNumberOfOutputs() * Output::kSize; // outputs_
      return size;
    }

    bool Write(const BufferPtr& buff) const{
      buff->PutLong(timestamp_);
      buff->PutLong(index_);
      buff->PutList(inputs_);
      buff->PutList(outputs_);
      return true;
    }

    bool Sign();
    bool VisitInputs(TransactionInputVisitor* vis) const;
    bool VisitOutputs(TransactionOutputVisitor* vis) const;

    std::string ToString() const{
      std::stringstream stream;
      stream << "Transaction(#" << GetIndex() << "," << GetNumberOfInputs() << " Inputs, " << GetNumberOfOutputs()
             << " Outputs)";
      return stream.str();
    }

    void operator=(const Transaction& other){
      timestamp_ = other.timestamp_;
      index_ = other.index_;
      inputs_ = other.inputs_;
      outputs_ = other.outputs_;
      //TODO: copy transaction signature
    }

    bool operator==(const Transaction& other){
      return timestamp_ == other.timestamp_
             && index_ == other.index_
             && inputs_ == other.inputs_
             && outputs_ == other.outputs_;
      //TODO: compare transaction signature
    }

    bool operator!=(const Transaction& other){
      return timestamp_ != other.timestamp_
             && index_ != other.index_
             && inputs_ != other.inputs_
             && outputs_ != other.outputs_;
      //TODO: compare transaction signature
    }

    static TransactionPtr NewInstance(int64_t index,
                                      const InputList& inputs,
                                      const OutputList& outputs,
                                      const Timestamp& timestamp = GetCurrentTimestamp()){
      return std::make_shared<Transaction>(index, inputs, outputs, timestamp);
    }

    static TransactionPtr NewInstance(const BufferPtr& buff){
      Timestamp timestamp = buff->GetLong();
      int64_t index = buff->GetLong();

      InputList inputs;

      int64_t idx;
      int64_t num_inputs = buff->GetLong();
      for(idx = 0; idx < num_inputs; idx++)
        inputs.push_back(Input(buff));

      OutputList outputs;
      int64_t num_outputs = buff->GetLong();
      for(idx = 0; idx < num_outputs; idx++)
        outputs.push_back(Output(buff));
      return std::make_shared<Transaction>(index, inputs, outputs, timestamp);
    }
  };

  typedef std::set<TransactionPtr, Transaction::DefaultComparator> TransactionList;

  class TransactionInputVisitor{
   protected:
    TransactionInputVisitor() = default;
   public:
    virtual ~TransactionInputVisitor() = default;
    virtual bool Visit(const Input& input) = 0;
  };

  class TransactionOutputVisitor{
   protected:
    TransactionOutputVisitor() = default;
   public:
    virtual ~TransactionOutputVisitor() = default;
    virtual bool Visit(const Output& output) = 0;
  };
}

#endif //TOKEN_TRANSACTION_H