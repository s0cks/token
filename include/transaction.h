#ifndef TOKEN_TRANSACTION_H
#define TOKEN_TRANSACTION_H

#include <set>

#include "json.h"
#include "hash.h"
#include "object.h"
#include "buffer.h"
#include "unclaimed_transaction.h"

namespace token{
  class Input : public SerializableObject{
    friend class Transaction;
   public:
    static inline int64_t
    GetSize(){
      return TransactionReference::kSize
             + User::GetSize();
    }
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
    ~Input(){}

    Type GetType() const{
      return Type::kInput;
    }

    /**
     * Returns the TransactionReference for this input.
     *
     * @see TransactionReference
     * @return The TransactionReference
     */
    TransactionReference& GetReference(){
      return reference_;
    }

    /**
     * Returns a copy of the TransactionReference for this input.
     *
     * @see TransactionReference
     * @return A copy of the TransactionReference
     */
    TransactionReference GetReference() const{
      return reference_;
    }

    /**
     * Returns the User for this input.
     *
     * @see User
     * @deprecated
     * @return The User for this input
     */
    User& GetUser(){
      return user_;
    }

    /**
     * Returns a copy of the User for this input.
     *
     * @see User
     * @deprecated
     * @return A copy of the User for this input
     */
    User GetUser() const{
      return user_;
    }

    /**
     * Returns the size of this object (in bytes).
     *
     * @return The size of this encoded object in bytes
     */
    int64_t GetBufferSize() const{
      return GetSize();
    }

    /**
     * Serializes this object to a Buffer.
     *
     * @param buffer - The Buffer to write to
     * @see Buffer
     * @return True when successful otherwise, false
     */
    bool Write(const BufferPtr& buffer) const{
      return buffer->PutReference(reference_)
             && buffer->PutUser(user_);
    }

    /**
     * Serializes this object to Json
     *
     * @param writer - The Json writer to use
     * @see JsonWriter
     * @see JsonString
     * @return True when successful otherwise, false
     */
    bool Write(Json::Writer& writer) const{
      return writer.StartObject()
          && writer.Key("transaction")
          && reference_.Write(writer)
          && writer.Key("user")
          && user_.Write(writer)
          && writer.EndObject();
    }

    /**
     * Returns the description of this object.
     *
     * @see Object::ToString()
     * @return The ToString() description of this object
     */
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

  class InputVisitor{
   protected:
    InputVisitor() = default;
   public:
    virtual ~InputVisitor() = default;
    virtual bool Visit(const Input& input) = 0;
  };

  class OutputVisitor{
   protected:
    OutputVisitor() = default;
   public:
    virtual ~OutputVisitor() = default;
    virtual bool Visit(const Output& output) = 0;
  };

  class Output : public SerializableObject{
    friend class Transaction;
   public:
    static inline int64_t
    GetSize(){
      return User::GetSize()
             + Product::GetSize();
    }
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
    Output(const char* user, const char* product):
      Output(User(user), Product(product)){}
    Output(const BufferPtr& buff):
      SerializableObject(),
      user_(buff->GetUser()),
      product_(buff->GetProduct()){}
    ~Output(){}

    Type GetType() const{
      return Type::kOutput;
    }

    /**
     * Returns the User for this Output.
     *
     * @see User
     * @return The User for this Output
     */
    User GetUser() const{
      return user_;
    }

    /**
     * Returns the Product for this Output.
     *
     * @see Product
     * @return The Product for this Output
     */
    Product GetProduct() const{
      return product_;
    }

    /**
     * Returns the size of this object (in bytes).
     *
     * @return The size of this encoded object in bytes
     */
    int64_t GetBufferSize() const{
      return User::GetSize()
             + Product::GetSize();
    }

    /**
     * Serializes this object to a Buffer.
     *
     * @param buffer - The Buffer to write to
     * @see Buffer
     * @return True when successful otherwise, false
     */
    bool Write(const BufferPtr& buff) const{
      return buff->PutUser(user_)
             && buff->PutProduct(product_);
    }

    /**
     * Serializes this object to Json
     *
     * @param writer - The Json writer to use
     * @see JsonWriter
     * @see JsonString
     * @return True when successful otherwise, false
     */
    bool Write(Json::Writer& writer) const{
      return writer.StartObject()
             && user_.Write(writer)
             && product_.Write(writer)
             && writer.EndObject();
    }

    /**
     * Returns the description of this object.
     *
     * @see Object::ToString()
     * @return The ToString() description of this object
     */
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

  class Transaction : public BinaryObject{
    friend class Block;
    friend class TransactionMessage;
   public:
    struct TimestampComparator{
      bool operator()(const TransactionPtr& a, const TransactionPtr& b){
        return a->timestamp_ < b->timestamp_;
      }
    };
   protected:
    Timestamp timestamp_;
    InputList inputs_;
    OutputList outputs_;
    std::string signature_;
   public:
    Transaction(const Timestamp& timestamp,
      const InputList& inputs,
      const OutputList& outputs):
      BinaryObject(),
      timestamp_(timestamp),
      inputs_(inputs),
      outputs_(outputs),
      signature_(){}
    ~Transaction() = default;

    Type GetType() const{
      return Type::kTransaction;
    }

    /**
     * Returns the Timestamp for when this transaction occurred (UTC).
     *
     * @see Timestamp
     * @return The Timestamp for when this transaction occurred (UTC)
     */
    Timestamp GetTimestamp() const{
      return timestamp_;
    }

#define DEFINE_CONTAINER_FUNCTIONS(Name, Type) \
    Type& Name(){ return Name##_; }            \
    Type Name() const{ return Name##_; }       \
    Type::iterator Name##_begin(){ return Name##_.begin(); } \
    Type::iterator Name##_end(){ return Name##_.end(); }     \
    Type::const_iterator Name##_begin() const{ return Name##_.begin(); } \
    Type::const_iterator Name##_end() const{ return Name##_.end(); }     \
    int64_t Name##_size() const{ return Name##_.size(); }
    DEFINE_CONTAINER_FUNCTIONS(inputs, InputList);
    DEFINE_CONTAINER_FUNCTIONS(outputs, OutputList);
#undef DEFINE_CONTAINER_FUNCTIONS

    /**
     * Returns the attached signature for the transaction.
     *
     * @return The signature for the transaction
     */
    std::string GetSignature() const{
      return signature_;
    }

    /**
     * Returns true if the transaction has been signed.
     *
     * @return True if the transaction has been signed otherwise, false
     */
    bool IsSigned() const{
      return !signature_.empty();
    }

    /**
     * Returns the size of this object (in bytes).
     *
     * @return The size of this encoded object in bytes
     */
    int64_t GetBufferSize() const{
      int64_t size = 0;
      size += sizeof(RawTimestamp); // timestamp_
      size += GetVectorSize(inputs_); // inputs_
      size += GetVectorSize(outputs_); // outputs_
      return size;
    }

    /**
     * Serializes this object to a Buffer.
     *
     * @param buffer - The Buffer to write to
     * @see Buffer
     * @return True when successful otherwise, false
     */
    bool Write(const BufferPtr& buff) const{
      return buff->PutTimestamp(timestamp_) // timestamp_
          && buff->PutVector(inputs_) // inputs_
          && buff->PutVector(outputs_); // outputs
    }

    /**
     * Serializes this object to Json
     *
     * @param writer - The Json writer to use
     * @see JsonWriter
     * @see JsonString
     * @return True when successful otherwise, false
     */
    bool Write(Json::Writer& writer) const{
      return writer.StartObject()
             && Json::SetField(writer, "timestamp", ToUnixTimestamp(timestamp_))
             && Json::SetField(writer, "inputs", inputs_)
             && Json::SetField(writer, "outputs", outputs_)
             && writer.EndObject();
    }

    /**
     * Compares 2 transactions for equality.
     *
     * @deprecated
     * @param tx - The other transaction
     * @return True if the 2 transactions are equal.
     */
    bool Equals(const TransactionPtr& tx) const{ // convert to Compare(tx1, tx2);
      if(timestamp_ != tx->timestamp_){
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

    /**
     * Signs a transaction using the local Keychain's KeyPair.
     *
     * @return True if the sign was successful otherwise, false
     */
    bool Sign();

    /**
     * Visit all of the inputs for this transaction.
     *
     * @param vis - The visitor to call for each input
     * @return True if the visit was successful otherwise, false
     */
    bool VisitInputs(InputVisitor* vis) const{
      for(auto& it : inputs_){
        if(!vis->Visit(it))
          return false;
      }
      return true;
    }

    /**
     * Visit all of the outputs for this transaction.
     *
     * @param vis - The visitor to call for each output
     * @return True if the visit was successful otherwise, false
     */
    bool VisitOutputs(OutputVisitor* vis) const{
      for(auto& it : outputs_){
        if(!vis->Visit(it))
          return false;
      }
      return true;
    }

    /**
     * Returns the description of this object.
     *
     * @see Object::ToString()
     * @return The ToString() description of this object
     */
    std::string ToString() const{
      std::stringstream ss;
      ss << "Transaction(";
      ss << "timestamp=" << FormatTimestampReadable(timestamp_) << ", ";
      ss << "inputs=[]" << ", "; //TODO: implement
      ss << "outputs=[]" << ","; //TODO: implement
      ss << ")";
      return ss.str();
    }

    static inline TransactionPtr
    NewInstance(const InputList& inputs, const OutputList& outputs, const Timestamp& timestamp = Clock::now()){
      return std::make_shared<Transaction>(timestamp, inputs, outputs);
    }

    static inline TransactionPtr
    FromBytes(const BufferPtr& buff){
      Timestamp timestamp = buff->GetTimestamp();

      InputList inputs;
      if(!buff->GetList(inputs)){
        LOG(WARNING) << "cannot read inputs from buffer of size " << buff->GetWrittenBytes();
        return nullptr;
      }

      OutputList outputs;
      if(!buff->GetList(outputs)){
        LOG(WARNING) << "cannot read outputs from buffer of size " << buff->GetWrittenBytes();
        return nullptr;
      }
      return std::make_shared<Transaction>(timestamp, inputs, outputs);
    }
  };

  typedef std::vector<TransactionPtr> TransactionList;
  typedef std::set<TransactionPtr, Transaction::TimestampComparator> TransactionSet;

  //TODO: rename?
  class IndexedTransaction : public Transaction{
   public:
    struct HashEqualsComparator{
      bool operator()(const IndexedTransactionPtr& a, const IndexedTransactionPtr& b){
        return a->GetHash() == b->GetHash();
      }
    };

    struct IndexComparator{
      bool operator()(const IndexedTransactionPtr& a, const IndexedTransactionPtr& b){
        return a->GetIndex() < b->GetIndex();
      }
    };
   private:
    int64_t index_;
   public:
    IndexedTransaction(const Timestamp& timestamp, const int64_t& index, const InputList& inputs, const OutputList& outputs):
        Transaction(timestamp, inputs, outputs),
        index_(index){}
    ~IndexedTransaction() = default;

    Type GetType() const{
      return Type::kIndexedTransaction;
    }

    int64_t GetIndex() const{
      return index_;
    }

    bool Write(const BufferPtr& buffer) const{
      return buffer->PutLong(index_)
          && Transaction::Write(buffer);
    }

    int64_t GetBufferSize() const{
      int64_t size = 0;
      size += sizeof(int64_t); // index_
      size += Transaction::GetBufferSize();
      return size;
    }

    std::string ToString() const{
      std::stringstream ss;
      ss << "IndexedTransaction(";
      ss << "index=" << index_ << ", ";
      ss << "timestamp=" << FormatTimestampReadable(timestamp_) << ", ";
      ss << "inputs=[]" << ", ";
      ss << "outputs=[]";
      ss << ")";
      return ss.str();
    }

    static inline IndexedTransactionPtr
    NewInstance(const int64_t& index, const InputList& inputs, const OutputList& outputs, const Timestamp& timestamp=Clock::now()){
      return std::make_shared<IndexedTransaction>(timestamp, index, inputs, outputs);
    }

    static inline IndexedTransactionPtr
    FromBytes(const BufferPtr& buffer){
      int64_t index = buffer->GetLong();
      Timestamp timestamp = buffer->GetTimestamp();

      InputList inputs;
      if(!buffer->GetList(inputs)){
        LOG(WARNING) << "cannot read input list from buffer of size " << buffer->GetWrittenBytes();
        return nullptr;
      }

      OutputList outputs;
      if(!buffer->GetList(outputs)){
        LOG(WARNING) << "cannot read output list from buffer of size " << buffer->GetWrittenBytes();
        return nullptr;
      }
      return std::make_shared<IndexedTransaction>(timestamp, index, inputs, outputs);
    }
  };

  typedef std::vector<IndexedTransactionPtr> IndexedTransactionList;
  typedef std::set<IndexedTransactionPtr, IndexedTransaction::IndexComparator> IndexedTransactionSet;
}

#endif //TOKEN_TRANSACTION_H