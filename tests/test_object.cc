#include "test_suite.h"
#include "object.h"
#include "utils/buffer.h"

namespace Token{
  static inline User
  CreateUserA(){
    // predictable
    return User("VenueA");
  }

  static inline User
  CreateUserB(){
    return User(Hash::GenerateNonce().HexString());
  }

  DEFINE_RAW_OBJECT_POSITIVE_TEST(User, CreateUserA);
  DEFINE_RAW_OBJECT_NEGATIVE_TEST(User, CreateUserA, CreateUserB);
  DEFINE_RAW_OBJECT_SERIALIZATION_TEST(User, CreateUserA);

  static inline Product
  CreateProductA(){
    // predictable
    return Product("TestToken");
  }

  static inline Product
  CreateProductB(){
    // unpredictable
    return Product(Hash::GenerateNonce().HexString());
  }

  DEFINE_RAW_OBJECT_POSITIVE_TEST(Product, CreateProductA);
  DEFINE_RAW_OBJECT_NEGATIVE_TEST(Product, CreateProductA, CreateProductB);
  DEFINE_RAW_OBJECT_SERIALIZATION_TEST(Product, CreateProductA);

  static inline UUID
  CreateUUIDA(){
    // predictable
    return UUID("12823be0-50e4-11eb-90d1-bbc02618ba32");
  }

  static inline UUID
  CreateUUIDB(){
    // unpredictable
    return UUID();
  }

  DEFINE_RAW_OBJECT_POSITIVE_TEST(UUID, CreateUUIDA);
  DEFINE_RAW_OBJECT_NEGATIVE_TEST(UUID, CreateUUIDA, CreateUUIDB);
  DEFINE_RAW_OBJECT_SERIALIZATION_TEST(UUID, CreateUUIDA);

  static inline Hash
  CreateHashA(){
    return Hash::FromHexString("1191F9F9657BB3AB6D6E043D7B4017507D802795FE58EFA3043A066980C32C72");
  }

  static inline Hash
  CreateHashB(){
    return Hash::GenerateNonce();
  }

  DEFINE_RAW_OBJECT_POSITIVE_TEST(Hash, CreateHashA);
  DEFINE_RAW_OBJECT_NEGATIVE_TEST(Hash, CreateHashA, CreateHashB);
  DEFINE_RAW_OBJECT_SERIALIZATION_TEST(Hash, CreateHashA);
}