#include "test_test.h"

namespace Token{
    TEST_F(TestTest, test_test){
        ASSERT_EQ(Token::GetVersion(), "1.0.0");
    }
}