#ifndef TOKEN_TRANSACTION_H
#define TOKEN_TRANSACTION_H

#include <set>

#include "codec.h"
#include "encoder.h"
#include "decoder.h"
#include "timestamp.h"
#include "binary_object.h"
#include "transaction_input.h"
#include "transaction_output.h"

namespace token{
  class Transaction : public BinaryObject, public std::enable_shared_from_this<Transaction>{
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

    class Encoder : public codec::EncoderBase<Transaction>{
     public:
      Encoder(const Transaction& value, const codec::EncoderFlags& flags=codec::kDefaultEncoderFlags):
        codec::EncoderBase<Transaction>(value, flags){}
      Encoder(const Encoder& other) = default;
      ~Encoder() override = default;

      int64_t GetBufferSize() const override;
      bool Encode(const BufferPtr& buff) const override;
      Encoder& operator=(const Encoder& other) = default;
    };

    class Decoder : public codec::DecoderBase<Transaction>{
     public:
      Decoder(const codec::DecoderHints& hints=codec::kDefaultDecoderHints):
        codec::DecoderBase<Transaction>(hints){}
      Decoder(const Decoder& other) = default;
      ~Decoder() override = default;
      bool Decode(const BufferPtr& buff, Transaction& result) const override;
      Decoder& operator=(const Decoder& other) = default;
    };
   protected:
    Timestamp timestamp_;
    InputList inputs_;
    OutputList outputs_;
    std::string signature_;
   public:
    Transaction() = default;
    Transaction(const Timestamp& timestamp,
      InputList inputs,
      OutputList  outputs):
      BinaryObject(),
      timestamp_(timestamp),
      inputs_(std::move(inputs)),
      outputs_(std::move(outputs)),
      signature_(){}
    Transaction(const Transaction& other) = default;
    ~Transaction() override = default;

    Type type() const override{
      return Type::kTransaction;
    }

    Timestamp& timestamp(){
      return timestamp_;
    }

    Timestamp timestamp() const{
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

    BufferPtr ToBuffer() const override;

    /**
     * Returns the description of this object.
     *
     * @see Object::ToString()
     * @return The ToString() description of this object
     */
    std::string ToString() const override;

    Transaction& operator=(const Transaction& other) = default;

    friend bool operator==(const Transaction& a, const Transaction& b){
      return a.timestamp_ == b.timestamp_
          && a.inputs_ == b.inputs_
          && a.outputs_ == b.outputs_;
    }

    friend bool operator!=(const Transaction& a, const Transaction& b){
      return !operator==(a, b);
    }

    friend bool operator<(const Transaction& a, const Transaction& b){
      return a.timestamp_ < b.timestamp_;
    }

    friend bool operator>(const Transaction& a, const Transaction& b){
      return a.timestamp_ > b.timestamp_;
    }

    static inline TransactionPtr
    NewInstance(const InputList& inputs, const OutputList& outputs, const Timestamp& timestamp = Clock::now()){
      return std::make_shared<Transaction>(timestamp, inputs, outputs);
    }

    static inline bool
    Decode(const BufferPtr& buff, Transaction& result, const codec::DecoderHints& hints=codec::kDefaultDecoderHints){
      Decoder decoder(hints);
      return decoder.Decode(buff, result);
    }

    static inline TransactionPtr
    DecodeNew(const BufferPtr& buff, const codec::DecoderHints& hints=codec::kDefaultDecoderHints){
      Transaction result;
      if(!Decode(buff, result, hints)){
        DLOG(WARNING) << "cannot decode Transaction.";
        return nullptr;
      }
      return std::make_shared<Transaction>(result);
    }
  };

  typedef std::vector<TransactionPtr> TransactionList;
  typedef std::set<TransactionPtr, Transaction::TimestampComparator> TransactionSet;
}

#endif //TOKEN_TRANSACTION_H