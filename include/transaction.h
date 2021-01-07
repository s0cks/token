#ifndef TOKEN_TRANSACTION_H
#define TOKEN_TRANSACTION_H

#include <set>
#include "hash.h"
#include "object.h"
#include "utils/buffer.h"
#include "utils/file_reader.h"
#include "utils/file_writer.h"
#include "unclaimed_transaction.h"

namespace Token{
  typedef std::shared_ptr<Transaction> TransactionPtr;

  class Input : public SerializableObject{
    friend class Transaction;
   public:
    static const int64_t kSize = TransactionReference::kSize + User::kSize;
   private:
    TransactionReference reference_;
    User user_;//TODO: remove field
   public:
    Input(const TransactionReference& ref, const User& user):
      SerializableObject(),
      reference_(ref),
      user_(user){}
    Input(const Hash& tx, int64_t index, const User& user):
      SerializableObject(),
      reference_(tx, index),
      user_(user){}
    Input(const Hash& tx, int64_t index, const std::string& user):
      SerializableObject(),
      reference_(tx, index),
      user_(user){}
    Input(const BufferPtr& buff):
      SerializableObject(),
      reference_(buff->GetReference()),
      user_(buff->GetUser()){}
    Input(BinaryFileReader* reader):
      SerializableObject(),
      reference_(reader->ReadHash(), reader->ReadLong()),
      user_(reader->ReadUser()){}
    ~Input(){}

    TransactionReference& GetReference(){
      return reference_;
    }

    TransactionReference GetReference() const{
      return reference_;
    }

    User& GetUser(){
      return user_;
    }

    User GetUser() const{
      return user_;
    }

    int64_t GetBufferSize() const{
      return Input::kSize;
    }

    bool Write(const BufferPtr& buffer) const{
      return buffer->PutReference(reference_)
          && buffer->PutUser(user_);
    }

    bool Write(BinaryFileWriter* writer) const{
      writer->WriteReference(reference_);
      writer->WriteUser(user_);
      return true;
    }

    std::string ToString() const{
      std::stringstream stream;
      stream << "Input(" << reference_ << ", " << GetUser() << ")";
      return stream.str();
    }

    friend std::ostream& operator<<(std::ostream& stream, const Input& input){
      return stream << input.ToString();
    }

    void operator=(const Input& other){
      reference_ = other.reference_;
      user_ = other.user_;
    }

    friend bool operator==(const Input& a, const Input& b){
      return a.reference_ == b.reference_
             && a.user_ == b.user_;
    }

    friend bool operator!=(const Input& a, const Input& b){
      return !operator==(a, b);
    }

    friend bool operator<(const Input& a, const Input& b){
      return a.reference_ < b.reference_;
    }
  };

  class Output : public SerializableObject{
    friend class Transaction;
   public:
    static const int64_t kSize = User::kSize + Product::kSize;
   private:
    User user_;
    Product product_;
   public:
    Output(const User& user, const Product& product):
      SerializableObject(),
      user_(user),
      product_(product){}
    Output(const std::string& user, const Product& product):
      Output(User(user), product){}
    Output(const std::string& user, const std::string& product):
      Output(User(user), Product(product)){}
    Output(const BufferPtr& buff):
      SerializableObject(),
      user_(buff->GetUser()),
      product_(buff->GetProduct()){}
    Output(BinaryFileReader* reader):
      SerializableObject(),
      user_(reader->ReadUser()),
      product_(reader->ReadProduct()){}
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

    bool Write(BinaryFileWriter* writer) const{
      writer->WriteUser(user_);
      writer->WriteProduct(product_);
      return true;
    }

    std::string ToString() const{
      std::stringstream stream;
      stream << "Output(" << GetUser() << "; " << GetProduct() << ")";
      return stream.str();
    }

    friend std::ostream& operator<<(std::ostream& stream, const Output& output){
      return stream << output.ToString();
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
      if(a.user_ == b.user_){
        return a.product_ < b.product_;
      }
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
          return true;
        } else if(a->timestamp_ > b->timestamp_){
          return false;
        }
        return a->index_ < b->index_;
      }
    };

    struct TimestampComparator{
      bool operator()(const TransactionPtr& a, const TransactionPtr& b){
        return a->timestamp_ < b->timestamp_;
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
    Transaction(const Timestamp& timestamp,
      const int64_t& index,
      const InputList& inputs,
      const OutputList& outputs):
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

    bool Write(BinaryFileWriter* writer) const{
      writer->WriteLong(timestamp_);
      writer->WriteLong(index_);
      writer->WriteList(inputs_);
      writer->WriteList(outputs_);
      return true;
    }

    bool Equals(const TransactionPtr& tx) const{
      if(timestamp_ != tx->timestamp_){
        return false;
      }
      if(index_ != tx->index_){
        return false;
      }
      if(!std::equal(
        inputs_.begin(),
        inputs_.end(),
        tx->inputs_.begin(),
        [](const Input& a, const Input& b){ return a == b; }
      )){
        return false;
      }
      if(!std::equal(
        outputs_.begin(),
        outputs_.end(),
        tx->outputs_.begin(),
        [](const Output& a, const Output& b){ return a == b; }
      )){
        return false;
      }
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

    static TransactionPtr NewInstance(const int64_t& index,
      const InputList& inputs,
      const OutputList& outputs,
      const Timestamp& timestamp = GetCurrentTimestamp());
    static TransactionPtr FromBytes(const BufferPtr& buff);
    static TransactionPtr NewInstance(BinaryFileReader* reader);
  };

  typedef std::vector<TransactionPtr> TransactionList;
  typedef std::set<TransactionPtr, Transaction::DefaultComparator> TransactionSet;

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