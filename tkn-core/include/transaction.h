#ifndef TOKEN_TRANSACTION_H
#define TOKEN_TRANSACTION_H

#include <set>
#include <utility>

#include "transaction.pb.h"

#include "object.h"
#include "builder.h"
#include "timestamp.h"
#include "transaction_input.h"
#include "transaction_output.h"

namespace token{
  namespace internal{
    class TransactionBase : public BinaryObject{
    protected:
      Timestamp timestamp_;
      InputList inputs_;
      OutputList outputs_;

      TransactionBase():
        timestamp_(Clock::now()),
        inputs_(),
        outputs_(){
      }
      TransactionBase(const Timestamp& timestamp, const InputList& inputs, const OutputList& outputs):
        timestamp_(timestamp),
        inputs_(inputs.begin(), inputs.end()),
        outputs_(outputs.begin(), outputs.end()){
      }

      template<class M>
      explicit TransactionBase(const M& raw):
        timestamp_(FromUnixTimestamp(raw.timestamp())),
        inputs_(),
        outputs_(){
        //TODO: fill inputs
        //TODO: fill outputs
      }
    public:
      TransactionBase(const TransactionBase& rhs) = default;
      ~TransactionBase() override = default;

      Timestamp timestamp() const{
        return timestamp_;
      }

      InputList inputs() const{
        return inputs_;
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

      uint64_t GetNumberOfInputs() const{
        return inputs_.size();
      }

      InputPtr GetInput(const uint64_t& idx) const{
        return inputs_[idx];
      }

      bool VisitInputs(InputVisitor* vis){
        for(auto& it : inputs_){
          if(!vis->Visit(it))
            return false;
        }
        return true;
      }

      OutputList outputs() const{
        return outputs_;
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

      uint64_t GetNumberOfOutputs() const{
        return outputs_.size();
      }

      OutputPtr GetOutput(const uint64_t& idx) const{
        return outputs_[idx];
      }

      bool VisitOutputs(OutputVisitor* vis){
        for(auto& it : outputs_){
          if(!vis->Visit(it))
            return false;
        }
        return true;
      }

      TransactionBase& operator=(const TransactionBase& rhs) = default;
    };
  }
}

#endif //TOKEN_TRANSACTION_H