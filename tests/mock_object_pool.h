#ifndef TOKEN_MOCK_OBJECT_POOL_H
#define TOKEN_MOCK_OBJECT_POOL_H

#include "test_suite.h"
#include "pool.h"

namespace token{
  class MockObjectPool;
  typedef std::shared_ptr<MockObjectPool> MockObjectPoolPtr;

  class MockObjectPool : public ObjectPool{
   public:
    MockObjectPool():
      ObjectPool(){}
    ~MockObjectPool() override = default;

#define DEFINE_MOCK_TYPE_METHODS(Name) \
    MOCK_METHOD(bool, Has##Name, (const Hash&), (const)); \
    MOCK_METHOD(bool, Has##Name##s, (), (const)); \
    MOCK_METHOD(Name##Ptr, Get##Name, (const Hash&), (const)); \
    MOCK_METHOD(bool, Put##Name, (const Hash&, const Name##Ptr&), (const));
    FOR_EACH_POOL_TYPE(DEFINE_MOCK_TYPE_METHODS)
#undef DEFINE_MOCK_TYPE_METHODS
  };

  static inline ObjectPoolPtr
  NewMockObjectPool(){
    return std::make_shared<MockObjectPool>();
  }
}

#endif//TOKEN_MOCK_OBJECT_POOL_H