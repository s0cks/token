#ifndef TOKEN_UUID_H
#define TOKEN_UUID_H

#include <uuid/uuid.h>
#include "utils/buffer.h"

namespace Token{
  class UUID{
   public:
    static const size_t kSize = 16;
   private:
    uuid_t uuid_;
   public:
    UUID():
        uuid_(){
      uuid_generate_time_safe(uuid_);
    }
    UUID(const std::string &uuid):
        uuid_(){
      uuid_parse(uuid.data(), uuid_);
    }
    UUID(const UUID &other):
        uuid_(){
      uuid_copy(uuid_, other.uuid_);
    }
    UUID(const BufferPtr &buff):
        uuid_(){
      if(!buff->GetBytes((uint8_t *) uuid_, kSize))
        LOG(WARNING) << "cannot read uuid from bytes";
    }
    ~UUID() = default;

    bool Write(const BufferPtr &buff) const{
      buff->PutBytes((uint8_t *) uuid_, kSize);
      return true;
    }

    std::string ToString() const{
      char uuid_str[37];
      uuid_unparse(uuid_, uuid_str);
      return std::string(uuid_str);
    }

    void operator=(const UUID &other){
      uuid_copy(uuid_, other.uuid_);
    }

    friend bool operator==(const UUID &a, const UUID &b){
      return uuid_compare(a.uuid_, b.uuid_) == 0;
    }

    friend bool operator!=(const UUID &a, const UUID &b){
      return uuid_compare(a.uuid_, b.uuid_) != 0;
    }

    friend bool operator<(const UUID &a, const UUID &b){
      return uuid_compare(a.uuid_, b.uuid_) < 0;
    }

    friend std::ostream &operator<<(std::ostream &stream, const UUID &uuid){
      stream << uuid.ToString();
      return stream;
    }
  };
}

#endif //TOKEN_UUID_H