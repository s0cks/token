#include "test_suite.h"

#include "utils/trie.h"
#include "http/router.h"

namespace Token{
    TEST(TestTrie, test){
        Trie<std::string, std::string, 28> trie;
        trie.Insert("Hello", "World");
        ASSERT_EQ(trie.Search("Hello"), "World");
    }
}