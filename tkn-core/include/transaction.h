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

        void SetTimestamp(const Timestamp& val) const{
          ProtoBuilder<T, M>::raw_.set_timestamp(val);
        }

        void AddInput(const Hash& hash, const uint64_t& index, const std::string& user, const std::string& product){
          auto new_input = ProtoBuilder<T, M>::raw_->add_inputs();
          new_input->set_hash(hash);
          new_input->set_index(index);
          new_input->set_user(user);
          new_input->set_product(product);
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
        BinaryObject(),
        raw_(std::move(raw)){}
      TransactionBase(const Timestamp& timestamp, const std::vector<Input>& inputs, const std::vector<Output>& outputs):
        TransactionBase(){
        raw_.set_timestamp(ToUnixTimestamp(timestamp));

        std::for_each(inputs.begin(), inputs.end(), [&](const Input& input){
          auto new_input = raw_.add_inputs();
          new_input->set_user(input.ToString());
          new_input->set_index(input.index());
          new_input->set_hash(input.transaction().HexString());
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

      TransactionBase& operator=(const TransactionBase& other) = default;
    };
  }
}

#endif //TOKEN_TRANSACTION_H