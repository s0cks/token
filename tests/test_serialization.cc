#include "block.h"
#include "test_serialization.h"

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
  TEST_F(TestSerialization, test_##Name##_rw){       \
    Type value = Generate##Name();               \
    ASSERT_TRUE(Write##Name(file_, value));       \
    ASSERT_TRUE(Seek(file_, 0));                  \
    ASSERT_EQ(Read##Name(file_), value);          \
  }
#define DEFINE_UNSIGNED_READWRITE_CASE(Name, Type) \
  TEST_F(TestSerialization, test_Unsigned##Name##_rw){ \
    u##Type value = GenerateUnsigned##Name();      \
    ASSERT_TRUE(WriteUnsigned##Name(file_, value)); \
    ASSERT_TRUE(Seek(file_, 0));                    \
    ASSERT_EQ(ReadUnsigned##Name(file_), value);    \
  }
#define DEFINE_READWRITE_CASE(Name, Type) \
  DEFINE_SIGNED_READWRITE_CASE(Name, Type)\
  DEFINE_UNSIGNED_READWRITE_CASE(Name, Type)

  FOR_EACH_RAW_TYPE(DEFINE_READWRITE_CASE)
#undef DEFINE_READWRITE_CASE
#undef DEFINE_SIGNED_READWRITE_CASE
#undef DEFINE_UNSIGNED_READWRITE_CASE

  TEST_F(TestSerialization, test_Hash_rw){
    Hash hash = Hash::GenerateNonce();
    ASSERT_TRUE(WriteHash(file_, hash));

    ASSERT_TRUE(Seek(file_, 0));
    ASSERT_EQ(ReadHash(file_), hash);
  }

  TEST_F(TestSerialization, test_Version_rw){
    Version expected(TOKEN_MAJOR_VERSION, TOKEN_MINOR_VERSION, TOKEN_REVISION_VERSION);
    ASSERT_TRUE(WriteVersion(file_, expected));
    ASSERT_TRUE(Seek(file_, 0));
    Version actual = ReadVersion(file_);
    ASSERT_EQ(actual, expected);
  }

  TEST_F(TestSerialization, test_ObjectTag_rw){
    ObjectTag expected(Type::kIndexedTransaction, 16424);
    ASSERT_TRUE(WriteTag(file_, expected));
    ASSERT_TRUE(Seek(file_, 0));
    ObjectTag actual = ReadTag(file_);
    ASSERT_EQ(actual, expected);
  }

//TODO: fix implementation
//  TEST_F(TestSerialization, test_Timestamp_rw){
//    Timestamp expected = Clock::now();
//    ASSERT_TRUE(WriteTimestamp(file_, expected));
//    ASSERT_TRUE(Seek(file_, 0));
//    Timestamp actual = ReadTimestamp(file_);
//    ASSERT_EQ(actual, expected);
//  }

  TEST_F(TestSerialization, test_Vector_rw){
    //TODO: implement
    LOG(WARNING) << "not implemented yet.";
  }

  TEST_F(TestSerialization, test_Set_rw){
    BlockPtr blk = Block::Genesis();
    IndexedTransactionSet& txs1 = blk->transactions();

    ASSERT_TRUE(WriteSet(file_, txs1));
    ASSERT_TRUE(Flush(file_));
    ASSERT_TRUE(Seek(file_, 0));

    IndexedTransactionSet txs2;
    ASSERT_TRUE(ReadSet(file_, Type::kIndexedTransaction, txs2));

    ASSERT_TRUE(std::equal(txs1.begin(), txs1.end(), txs2.begin(), IndexedTransaction::HashEqualsComparator()));
  }
}