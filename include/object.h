#ifndef TOKEN_OBJECT_H
#define TOKEN_OBJECT_H

#include "type.h"

namespace token{
  class Object{
   protected:
    Object() = default;
   public:
    virtual ~Object() = default;
    virtual Type type() const = 0;
    virtual std::string ToString() const = 0;
  };
}

#endif //TOKEN_OBJECT_H