#include "test_suite.h"
#include "utils/filesystem.h"

namespace token{
  static const int8_t kByteMinValue = -128;
  static const int8_t kByteMaxValue = 127;

  static const uint8_t kUnsignedByteMinValue = 0;
  static const uint8_t kUnsignedByteMaxValue = 0;

#define DEFINE_GENERATE_SIGNED(Name, Type) \
  static inline Type                       \
  Generate##Name(){                        \
    return (Type)(rand() % (kByteMaxValue+((Type)1)-kByteMinValue)+kByteMinValue); \
  }

#define DEFINE_GENERATE_UNSIGNED(Name, Type) \
  static inline u##Type                      \
  GenerateUnsigned##Name(){                  \
    return (Type)(rand() % (kUnsignedByteMaxValue+((u##Type)1)-kUnsignedByteMinValue)+kUnsignedByteMinValue); \
  }

#define DEFINE_GENERATE(Name, Type) \
  DEFINE_GENERATE_SIGNED(Name, Type)\
  DEFINE_GENERATE_UNSIGNED(Name, Type)

  FOR_EACH_RAW_TYPE(DEFINE_GENERATE)
#undef DEFINE_GENERATE
#undef DEFINE_GENERATE_SIGNED
#undef DEFINE_GENERATE_UNSIGNED

#define DEFINE_SIGNED_READWRITE_CASE(Name, Type) \
  TEST(TestSerialization, test_##Name##_rw){       \
    Type value = Generate##Name();               \
    FILE* file = tmpfile();                      \
    ASSERT_NE(file, nullptr);                    \
    ASSERT_TRUE(Write##Name(file, value));       \
    ASSERT_TRUE(Seek(file, 0));                  \
    ASSERT_EQ(Read##Name(file), value);          \
  }
#define DEFINE_UNSIGNED_READWRITE_CASE(Name, Type) \
  TEST(TestSerialization, test_Unsigned##Name##_rw){ \
    u##Type value = GenerateUnsigned##Name();      \
    FILE* file = tmpfile();                        \
    ASSERT_NE(file, nullptr);                      \
    ASSERT_TRUE(WriteUnsigned##Name(file, value)); \
    ASSERT_TRUE(Seek(file, 0));                    \
    ASSERT_EQ(ReadUnsigned##Name(file), value);    \
  }
#define DEFINE_READWRITE_CASE(Name, Type) \
  DEFINE_SIGNED_READWRITE_CASE(Name, Type)\
  DEFINE_UNSIGNED_READWRITE_CASE(Name, Type)

  FOR_EACH_RAW_TYPE(DEFINE_READWRITE_CASE)
#undef DEFINE_READWRITE_CASE
#undef DEFINE_SIGNED_READWRITE_CASE
#undef DEFINE_UNSIGNED_READWRITE_CASE

  TEST(TestSerialization, test_Hash_rw){
    Hash hash = Hash::GenerateNonce();
    FILE* file = tmpfile();
    ASSERT_NE(file, nullptr);
    ASSERT_TRUE(WriteHash(file, hash));

    ASSERT_TRUE(Seek(file, 0));
    ASSERT_EQ(ReadHash(file), hash);
  }

  TEST(TestSerialization, test_Version_rw){
    Version version = Version(TOKEN_MAJOR_VERSION, TOKEN_MINOR_VERSION, TOKEN_REVISION_VERSION);
    FILE* file = tmpfile();
    ASSERT_NE(file, nullptr);
    ASSERT_TRUE(WriteVersion(file, version));
    ASSERT_TRUE(Seek(file, 0));
    ASSERT_EQ(ReadVersion(file), version);
  }

  TEST(TestSerialization, test_ObjectTag_rw){
    ObjectTag tag(Type::kBlock, 12974);
    FILE* file = tmpfile();
    ASSERT_NE(file, nullptr);
    ASSERT_TRUE(WriteTag(file, tag));
    ASSERT_TRUE(Seek(file, 0));
    ASSERT_EQ(ReadTag(file), tag);
  }

  TEST(TestSerialization, test_Timestamp_rw){
    Timestamp timestamp = Clock::now();
    FILE* file = tmpfile();
    ASSERT_NE(file, nullptr);
    ASSERT_TRUE(WriteTimestamp(file, timestamp));
    ASSERT_TRUE(Seek(file, 0));
    ASSERT_EQ(ReadTimestamp(file), timestamp);
  }
}