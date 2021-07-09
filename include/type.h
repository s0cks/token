#ifndef TOKEN_TYPE_H
#define TOKEN_TYPE_H

#include <vector>
#include <memory>
#include <ostream>

namespace token{
#define FOR_EACH_NATIVE_TYPE(V) \
  V(Byte, int8)                 \
  V(Short, int16)               \
  V(Int, int32)                 \
  V(Long, int64)

#define FOR_EACH_MESSAGE_TYPE(V) \
  V(Version)                     \
  V(Verack)                      \
  V(Block)                       \
  V(Transaction)                 \
  V(Prepare)                     \
  V(Promise)                     \
  V(Commit)                      \
  V(Accepts)                     \
  V(Accepted)                    \
  V(Rejected)

#define FOR_EACH_TYPE(V) \
  V(Block)               \
  V(Input)               \
  V(Output)              \
  V(UnsignedTransaction)         \
  V(SignedTransaction)   \
  V(IndexedTransaction)  \
  V(UnclaimedTransaction)\
  V(Proposal)            \
  V(Buffer)              \
  V(Reference)

//TODO: remove FORWARD_DECLARE
#define FORWARD_DECLARE(Name) \
  class Name;                 \
  typedef std::shared_ptr<Name> Name##Ptr;
  FOR_EACH_TYPE(FORWARD_DECLARE)
#undef FORWARD_DECLARE

//TODO: remove FORWARD_DECLARE
  namespace rpc{
#define FORWARD_DECLARE(Name) \
    class Name##Message;      \
    typedef std::shared_ptr<Name##Message> Name##MessagePtr;
    FOR_EACH_MESSAGE_TYPE(FORWARD_DECLARE)
#undef FORWARD_DECLARE
  }

#define FOR_EACH_COMPONENT(V) \
  V(BlockChain)               \
  V(WalletManager)            \
  V(ObjectPool)               \
  V(ReferenceDatabase)

//TODO: remove FORWARD_DECLARE
#define FORWARD_DECLARE(Name) \
  class Name;                 \
  typedef std::shared_ptr<Name> Name##Ptr;
  FOR_EACH_COMPONENT(FORWARD_DECLARE)
#undef FORWARD_DECLARE

  enum class Type{
    kUnknown = 0,
#define DEFINE_TYPE(Name) k##Name,
    FOR_EACH_TYPE(DEFINE_TYPE)
#undef DEFINE_TYPE

#define DEFINE_TYPE(Name) k##Name##Message,
    FOR_EACH_MESSAGE_TYPE(DEFINE_TYPE)
#undef DEFINE_TYPE

    kMerkleNode,
    kBlockHeader,
    kHttpRequest,
    kHttpResponse,
  };

  static inline std::ostream&
  operator<<(std::ostream& stream, const Type& type){
    switch(type){
#define DEFINE_TOSTRING(Name) \
      case Type::k##Name:     \
        return stream << #Name;
      FOR_EACH_TYPE(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
#define DEFINE_TOSTRING(Name) \
      case Type::k##Name##Message: \
        return stream << #Name;
      FOR_EACH_MESSAGE_TYPE(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
      case Type::kHttpRequest:
        return stream << "HttpRequest";
      case Type::kHttpResponse:
        return stream << "HttpResponse";
      default:
        return stream << "Unknown";
    }
  }

  typedef std::vector<Input> InputList;
  typedef std::vector<Output> OutputList;
}

#endif//TOKEN_TYPE_H