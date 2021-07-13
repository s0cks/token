#ifndef TOKEN_INVENTORY_H
#define TOKEN_INVENTORY_H

#include <set>
#include <ostream>
#include "object.h"

#include "pool.h"
#include "blockchain.h"

namespace token{
  class InventoryItem{
   public:
    static inline int
    CompareType(const InventoryItem& a, const InventoryItem& b){
      if(a.GetType() < b.GetType()){
        return -1;
      } else if(a.GetType() > b.GetType()){
        return +1;
      }
      return 0;
    }

    static inline int
    CompareHash(const InventoryItem& a, const InventoryItem& b){
      if(a.GetHash() < b.GetHash()){
        return -1;
      } else if(a.GetHash() > b.GetHash()){
        return +1;
      }
      return 0;
    }

    static inline int
    Compare(const InventoryItem& a, const InventoryItem& b){
      int result;
      if((result = CompareType(a, b)) != 0)
        return result;
      return CompareHash(a, b);
    }

    struct Comparator{
      bool operator()(const InventoryItem& a, const InventoryItem& b) const{
        return Compare(a, b) == 0;// ?
      }
    };
   private:
    Type type_;
    Hash hash_;
   public:
    InventoryItem():
      type_(Type::kUnknown),
      hash_(){}
    InventoryItem(Type type, const Hash& hash):
      type_(type),
      hash_(hash){}
    explicit InventoryItem(const BufferPtr& buffer):
      type_(static_cast<Type>(buffer->GetInt())),
      hash_(buffer->GetHash()){}
    InventoryItem(const InventoryItem& item) = default;
    ~InventoryItem() = default;

    Type GetType() const{
      return type_;
    }

    Hash GetHash() const{
      return hash_;
    }

    bool IsBlock() const{
      return type_ == Type::kBlock;
    }

    bool IsTransaction() const{
      return type_ == Type::kUnsignedTransaction;
    }

    int64_t GetBufferSize() const{
      return GetSize();
    }

    bool Write(const BufferPtr& buffer) const{
      return buffer->PutInt(static_cast<int32_t>(type_))
          && buffer->PutHash(hash_);
    }

    std::string ToString() const{
      std::stringstream ss;
      ss << "InventoryItem(";
      ss << "type=" << type_ << ", ";
      ss << "hash=" << hash_;
      ss << ")";
      return ss.str();
    }

    InventoryItem& operator=(const InventoryItem& item) = default;

    friend bool operator==(const InventoryItem& a, const InventoryItem& b){
      return CompareType(a, b) == 0
          && CompareHash(a, b) == 0;
    }

    friend bool operator!=(const InventoryItem& a, const InventoryItem& b){
      return !operator==(a, b);
    }

    friend std::ostream& operator<<(std::ostream& stream, const InventoryItem& item){
      return stream << item.ToString();
    }

    static inline InventoryItem
    Of(const BlockHeader& val){
      return InventoryItem(Type::kBlock, val.hash());
    }

    static inline InventoryItem
    Of(const BlockPtr& val){
      return InventoryItem(Type::kBlock, val->hash());
    }

    static inline InventoryItem
    Of(const UnsignedTransactionPtr& val){
      return InventoryItem(Type::kUnsignedTransaction, val->hash());
    }

    static inline int64_t
    GetSize(){
      int64_t size = 0;
      size += sizeof(int32_t);
      size += Hash::GetSize();
      return size;
    }
  };

  typedef std::set<InventoryItem, InventoryItem::Comparator> InventoryItems;

  static inline InventoryItems&
  operator<<(InventoryItems& items, const InventoryItem& item){
    if(!items.insert(item).second)
      LOG(WARNING) << "cannot add item " << item << " to InventoryItems";
    return items;
  }

  static inline std::ostream&
  operator<<(std::ostream& stream, const InventoryItems& items){
    stream << "[";
    int64_t idx = 0;
    for(auto& item : items){
      stream << item;
      if(idx++ < (int64_t)items.size())
        stream << ", ";
    }
    stream << "]";
    return stream;
  }

  static inline bool
  Encode(const BufferPtr& buffer, const InventoryItems& items){
    return buffer->PutSet(items);
  }

  static inline bool
  Decode(const BufferPtr& buffer, InventoryItems& items){
    return buffer->GetSet(items);
  }
}

#endif//TOKEN_INVENTORY_H