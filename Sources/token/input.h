#ifndef TOKEN_TRANSACTION_INPUT_H
#define TOKEN_TRANSACTION_INPUT_H

#include <vector>

#include "hash.h"
#include "object.h"
#include "transaction_reference.h"

namespace token{
 class Input;
 class InputVisitor{
  protected:
   InputVisitor() = default;
  public:
   virtual ~InputVisitor() = default;
   virtual void Visit(const Input& val) = 0;
 };

 class Input : public SerializableObject{
  public:
   static constexpr const uint64_t kSize = TransactionReference::kSize;

   static inline int
   Compare(const Input& lhs, const Input& rhs){
     return TransactionReference::Compare(lhs.source(), rhs.source());
   }
  private:
   TransactionReference source_;
  public:
   Input() = default;
   explicit Input(const TransactionReference& source):
    source_(source){
   }
   Input(const uint256& hash, uint64_t index):
    source_(hash, index){
   }
   explicit Input(const BufferPtr& data):
    Input(data->GetTransactionReference()){
   }
   Input(const Input& rhs) = default;
   ~Input() override = default;

   Type type() const override{
     return Type::kInput;
   }

   TransactionReference source() const{
     return source_;
   }

   uint64_t GetBufferSize() const override{
     return kSize;
   }

   bool WriteTo(const BufferPtr& data) const override{
     return data->PutTransactionReference(source_);
   }

   Input& operator=(const Input& rhs) = default;

   friend std::ostream& operator<<(std::ostream& stream, const Input& val){
     return stream << "Input(source=" << val.source() << ")";
   }

   friend bool operator==(const Input& lhs, const Input& rhs){
     return Compare(lhs, rhs) == 0;
   }

   friend bool operator!=(const Input& lhs, const Input& rhs){
     return Compare(lhs, rhs) != 0;
   }

   friend bool operator<(const Input& lhs, const Input& rhs){
     return Compare(lhs, rhs) < 0;
   }

   friend bool operator>(const Input& lhs, const Input& rhs){
     return Compare(lhs, rhs) > 0;
   }
 };

#ifdef TOKEN_JSON_EXPORT
 namespace json{
  static inline bool
  Write(Writer& writer, const Input& val){//TODO: better error handling
    JSON_START_OBJECT(writer);
    {
      JSON_KEY(writer, "hash");
      JSON_STRING(writer, val.hash().HexString());
      if(!json::SetField(writer, "source", val.source()))
        return false;
    }
    JSON_END_OBJECT(writer);
    return true;
  }

  static inline bool
  SetField(Writer& writer, const char* name, const Input& val){
    JSON_KEY(writer, name);
    return Write(writer, val);
  }
 }
#endif//TOKEN_JSON_EXPORT
}

#endif//TOKEN_TRANSACTION_INPUT_H