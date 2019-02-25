#include <gtest/gtest.h>
#include "token.h"
#include "../src/array.h"

TEST(ArrayTests, TestEq){
    Token::Array<int> a(0xA);
    a.Add(0xA);
    a.Add(0xB);
    a.Add(0xC);

    Token::Array<int> b(0xA);
    b.Add(0xA);
    b.Add(0xB);
    b.Add(0xC);
    
    Token::Array<int> c(a);

    ASSERT_EQ(a, b);
    ASSERT_EQ(c, b);
}

TEST(SqRootTest, PositiveNos){
    ASSERT_EQ(true, true);
}

int
main(int argc, char** argv){
    testing::InitGoogleTest(&argc, argv);
    std::cout << Token::GetVersion() << std::endl;
    return RUN_ALL_TESTS();
}