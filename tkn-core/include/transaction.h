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

  namespace internal{
    template<class M>
    class TransactionBase : public BinaryObject{
    protected:
      template<class T>
      class TransactionBuilderBase : public internal::ProtoBuilder<T, M>{
      public:
        explicit TransactionBuilderBase(M* raw):
          internal::ProtoBuilder<T, M>(raw){}
        TransactionBuilderBase() = default;
        ~TransactionBuilderBase() = default;

        void SetTimestamp(const Timestamp& val){
          ProtoBuilder<T, M>::raw_->set_timestamp(ToUnixTimestamp(val));
        }

        void AddInput(const Hash& hash, const Hash& transaction, const uint64_t& index){
          auto new_input = ProtoBuilder<T, M>::raw_->add_inputs();
          new_input->set_index(index);
          new_input->set_transaction(transaction.HexString());
          new_input->set_hash(hash.HexString());
        }

        void AddOutput(const std::string& user, const std::string& product){
          auto new_output = ProtoBuilder<T, M>::raw_->add_outputs();
          new_output->set_user(user);
          new_output->set_product(product);
        }
      };

      M raw_;

      TransactionBase():
        BinaryObject(),
        raw_(){}
      explicit TransactionBase(M raw):
        TransactionBase(){
        raw_.CopyFrom(raw);
      }
      explicit TransactionBase(const internal::BufferPtr& data):
        TransactionBase(){
        auto length = static_cast<int>(data->length());
        if(!raw_.ParseFromArray(data->data(), length))
          LOG(FATAL) << "cannot parse from buffer of size: " << length;
      }
      TransactionBase(const Timestamp& timestamp, const std::vector<Input>& inputs, const std::vector<Output>& outputs):
        TransactionBase(){
        raw_.set_timestamp(ToUnixTimestamp(timestamp));

        std::for_each(inputs.begin(), inputs.end(), [&](const Input& input){
          auto new_input = raw_.add_inputs();
          auto source = input.source();
          new_input->set_hash(input.hash().HexString());
          new_input->set_transaction(source.transaction().HexString());
          new_input->set_index(source.index());
        });

        std::for_each(outputs.begin(), outputs.end(), [&](const Output& val){
          auto new_output = raw_.add_outputs();
          new_output->set_user(val.user().ToString());
          new_output->set_product(val.product().ToString());
        });
      }
    public:
      TransactionBase(const TransactionBase& other) = default;
      ~TransactionBase() override = default;

      Timestamp timestamp() const{
        return FromUnixTimestamp(raw_.timestamp());
      }

      virtual bool IsSigned() const{
        return false;
      }

      void GetInputs(std::vector<Input>& results){

      }

      void GetOutputs(std::vector<Output>& results){
        auto outputs = raw_.outputs();
        std::for_each(outputs.begin(), outputs.end(), [&](internal::proto::Output& val){
          results.emplace_back(val);
        });
      }

      bool VisitInputs(InputVisitor* vis) const{
        DLOG(INFO) << "visiting " << raw_.inputs_size() << " inputs....";
        for(auto it : raw_.inputs()){
          Input val(it);
          DLOG(INFO) << "visiting: " << val;
        }
        return true;
      }

      bool VisitOutputs(OutputVisitor* vis) const{
        for(auto& it : raw_.outputs()){
          Output val(it);
          if(!vis->Visit(val))
            return false;
        }
        return true;
      }

      TransactionBase& operator=(const TransactionBase& other) = default;
    };
  }
}

#endif //TOKEN_TRANSACTION_H