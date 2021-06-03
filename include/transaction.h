#ifndef TOKEN_TRANSACTION_H
#define TOKEN_TRANSACTION_H

#include <set>
#include "object.h"
#include "timestamp.h"
#include "transaction_input.h"
#include "transaction_output.h"

namespace token{
  class Transaction : public BinaryObject{
    friend class Block;
    friend class TransactionMessage;
   public:
    struct TimestampComparator{
      bool operator()(const TransactionPtr& a, const TransactionPtr& b){
        return a->timestamp_ < b->timestamp_;
      }
    };

    class InputVisitor{
     protected:
      InputVisitor() = default;
     public:
      virtual ~InputVisitor() = default;
      virtual bool Visit(const Input& val) = 0;
    };

    class OutputVisitor{
     protected:
      OutputVisitor() = default;
     public:
      virtual ~OutputVisitor() = default;
      virtual bool Visit(const Output& val) = 0;
    };
   protected:
    Timestamp timestamp_;
    InputList inputs_;
    OutputList outputs_;
    std::string signature_;
   public:
    Transaction(const Timestamp& timestamp,
      InputList inputs,
      OutputList  outputs):
      BinaryObject(),
      timestamp_(timestamp),
      inputs_(std::move(inputs)),
      outputs_(std::move(outputs)),
      signature_(){}
    ~Transaction() override = default;

    Type GetType() const override{
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
    int64_t GetBufferSize() const override{
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
    bool Write(const BufferPtr& buff) const override;

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
    std::string ToString() const override{
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

    static TransactionPtr FromBytes(const BufferPtr& buff);
  };

  typedef std::vector<TransactionPtr> TransactionList;
  typedef std::set<TransactionPtr, Transaction::TimestampComparator> TransactionSet;
}

#endif //TOKEN_TRANSACTION_H