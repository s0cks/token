#ifndef TOKEN_TRANSACTION_INPUT_H
#define TOKEN_TRANSACTION_INPUT_H

#include <vector>
#include "user.h"
#include "object.h"

namespace token{
  class Input : public SerializableObject{
    friend class Transaction;
   public:
    static inline int64_t
    GetSize(){
      return TransactionReference::kSize
          + kUserSize;
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
    explicit Input(const BufferPtr& buff);
    Input(const Input& other) = default;
    ~Input() override = default;

    Type GetType() const override{
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
    int64_t GetBufferSize() const override{
      return GetSize();
    }

    /**
     * Serializes this object to a Buffer.
     *
     * @param buffer - The Buffer to write to
     * @see Buffer
     * @return True when successful otherwise, false
     */
    bool Write(const BufferPtr& buffer) const override;

    /**
     * Returns the description of this object.
     *
     * @see Object::ToString()
     * @return The ToString() description of this object
     */
    std::string ToString() const override;

    Input& operator=(const Input& other) = default;

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

    friend bool operator>(const Input& a, const Input& b){
      return a.reference_ > b.reference_;
    }

    friend std::ostream& operator<<(std::ostream& stream, const Input& input){
      return stream << input.ToString();
    }

    static Input ParseFrom(const BufferPtr& buff);
  };

  typedef std::vector<Input> InputList;
}

#endif//TOKEN_TRANSACTION_INPUT_H