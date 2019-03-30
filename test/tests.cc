#include <gtest/gtest.h>
#include <sstream>
#include <string>
#include "token.h"
#include "user.h"
#include "blockchain.h"

std::string GetGenesisPreviousHash(){
    std::stringstream stream;
    for(int i = 0; i <= 64; i++) stream << "F";
    return stream.str();
}

TEST(InputEq, TestEq){
    Token::Input in1(GetGenesisPreviousHash(), 0);
    std::cout << in1 << std::endl;
    Token::Input in2(GetGenesisPreviousHash(), 1);
    std::cout << in2 << std::endl;

    ASSERT_TRUE(in1 == in1);
    ASSERT_TRUE(in2 == in2);
    ASSERT_FALSE(in1 == in2);
}

TEST(OutputEq, TestEq){
    Token::Output out1("TestToken", "TestUser", 0);
    std::cout << out1 << std::endl;
    Token::Output out2("TestToken2", "TestUser2", 0);
    std::cout << out2 << std::endl;

    ASSERT_TRUE(out1 == out1);
    ASSERT_TRUE(out2 == out2);
    ASSERT_FALSE(out1 == out2);
}

TEST(InputEncodeDecode, TestEq){
    Token::Input in1(GetGenesisPreviousHash(), 0);
    std::cout << in1 << std::endl;
    Token::Input in2(GetGenesisPreviousHash(), 1);
    std::cout << in2 << std::endl;

    Token::ByteBuffer bb;
    in1.Encode(&bb);

    Token::Input* in3 = Token::Input::Decode(&bb);
    std::cout << (*in3) << std::endl;
    ASSERT_TRUE(in1 == in1);
    ASSERT_TRUE(in2 == in2);
    ASSERT_FALSE(in1 == in2);
    ASSERT_TRUE(in1 == (*in3));
    ASSERT_FALSE((*in3) == in2);
}

TEST(OutputEncodeDecode, TestEq){
    Token::Output out1("TestToken", "TestUser", 0);
    std::cout << out1 << std::endl;
    Token::Output out2("TestToken2", "TestUser2", 0);
    std::cout << out2 << std::endl;

    Token::ByteBuffer bb;
    out1.Encode(&bb);

    Token::Output* out3 = Token::Output::Decode(&bb);
    std::cout << (*out3) << std::endl;

    ASSERT_TRUE(out1 == out1);
    ASSERT_TRUE(out2 == out2);
    ASSERT_FALSE(out1 == out2);
    ASSERT_TRUE(out1 == (*out3));
    ASSERT_FALSE((*out3) == out2);
}

TEST(TransactionEq, TestEq){
    Token::Transaction tx1(0);
    tx1.AddInput(GetGenesisPreviousHash(), 0);
    tx1.AddOutput("TestUser", "TestToken1");
    std::cout << tx1 << std::endl;

    Token::Transaction tx2(0);
    tx2.AddInput(GetGenesisPreviousHash(), 1);
    tx2.AddOutput("TestUser2", "TestToken2");
    std::cout << tx2 << std::endl;

    ASSERT_TRUE(tx1 == tx1);
    ASSERT_TRUE(tx2 == tx2);
    ASSERT_FALSE(tx1 == tx2);
}

TEST(TransactionEncodeDecode, TestEq){
    Token::Transaction tx1(0);
    tx1.AddInput(GetGenesisPreviousHash(), 0);
    tx1.AddOutput("TestUser", "TestToken1");
    std::cout << tx1 << std::endl;

    Token::Transaction tx2(0);
    tx2.AddInput(GetGenesisPreviousHash(), 1);
    tx2.AddOutput("TestUser2", "TestToken2");
    std::cout << tx2 << std::endl;

    ASSERT_TRUE(tx1 == tx1);
    ASSERT_TRUE(tx2 == tx2);
    ASSERT_FALSE(tx1 == tx2);

    Token::ByteBuffer bb;
    tx1.Encode(&bb);
    Token::Transaction* tx3 = Token::Transaction::Decode(&bb);
    ASSERT_TRUE((*tx3) == tx1);
    ASSERT_FALSE((*tx3) == tx2);
}

TEST(TransactionHash, TestEq){
    Token::Transaction tx1(0);
    tx1.AddInput(GetGenesisPreviousHash(), 0);
    tx1.AddOutput("TestUser", "TestToken");
    std::cout << tx1 << std::endl;

    Token::Transaction tx2(0);
    tx2.AddInput(GetGenesisPreviousHash(), 0);
    tx2.AddOutput("TestUser2", "TestToken2");
    std::cout << tx2 << std::endl;

    std::cout << "Transaction #1: " << tx1.GetHash() << std::endl;
    std::cout << "Transaction #2: " << tx2.GetHash() << std::endl;
    ASSERT_TRUE(tx1.GetHash() == tx1.GetHash());
    ASSERT_FALSE(tx1.GetHash() == tx2.GetHash());

    Token::ByteBuffer bb;
    tx1.Encode(&bb);
    Token::Transaction* tx3 = Token::Transaction::Decode(&bb);
    std::cout << (*tx3) << std::endl;

    std::cout << "Transaction #3: " << tx3->GetHash() << std::endl;
    ASSERT_TRUE(tx3->GetHash() == tx1.GetHash());
    ASSERT_FALSE(tx3->GetHash() == tx2.GetHash());
}

TEST(BlockEq, TestEq){
    Token::Block block1(GetGenesisPreviousHash(), 0);
    Token::Transaction* b1tx1 = block1.CreateTransaction();
    b1tx1->AddInput(GetGenesisPreviousHash(), 0);
    b1tx1->AddOutput("TestUser", "TestToken");
    std::cout << block1 << std::endl;

    Token::Block block2(GetGenesisPreviousHash(), 1);
    Token::Transaction* b2tx1 = block2.CreateTransaction();
    b2tx1->AddInput(GetGenesisPreviousHash(), 0);
    b2tx1->AddOutput("TestUser2", "TestToken2");
    std::cout << block2 << std::endl;

    ASSERT_TRUE(block1 == block1);
    ASSERT_TRUE(block2 == block2);
    ASSERT_FALSE(block1 == block2);

    Token::ByteBuffer bb;
    block1.Encode(&bb);
    Token::Block* block3 = Token::Block::Decode(&bb);
    std::cout << (*block3) << std::endl;

    ASSERT_TRUE((*block3) == block1);
    ASSERT_FALSE((*block3) == block2);
}

TEST(BlockChainTest, TestBlockChain){
    Token::User user("TestUser");
    Token::BlockChain* chain = Token::BlockChain::CreateInstance(&user);

    Token::Block* nblock = chain->CreateBlock();
    Token::UnclaimedTransactionPool* utxos = chain->GetHeadUnclaimedTransactionPool();
    std::cout << (*utxos) << std::endl;

    Token::Transaction* tx = nblock->CreateTransaction();
    std::string unspent = chain->GetHead()->GetCoinbaseTransaction()->GetHash();
    std::cout << "Spending unspent transaction: " << unspent << std::endl;
    tx->AddInput(unspent, 0);
    tx->AddOutput("TestUser2", "TestToken");

    ASSERT_TRUE(chain->Append(nblock));

    nblock = chain->CreateBlock();
    tx = nblock->CreateTransaction();
    tx->AddInput(nblock->GetHash(), 0);
    tx->AddOutput("TestUser2", "TestToken");
    ASSERT_FALSE(chain->Append(nblock));
}

TEST(BlockChainSpendTest, TestBlockChain){
    Token::User user2("TestUser2");
    Token::User user("TestUser");
    Token::BlockChain* chain = Token::BlockChain::CreateInstance(&user);
    // Genesis := 0-128 Unspent Tokens

    Token::Block* valid_block = chain->CreateBlock();
    Token::Transaction* valid_tx = valid_block->CreateTransaction();
    valid_tx->AddInput(chain->GetHead()->GetCoinbaseTransaction()->GetHash(), 100);
    valid_tx->AddInput(chain->GetHead()->GetCoinbaseTransaction()->GetHash(), 17);
    valid_tx->AddOutput(user2.GetUserId(), "TestToken");

    ASSERT_TRUE(chain->Append(valid_block));

    std::cout << "<HEAD>" << std::endl;
    std::cout << (*chain->GetHead()) << std::endl;
    std::cout << (*chain->GetHeadUnclaimedTransactionPool()) << std::endl;

    valid_block = chain->CreateBlock();
    valid_tx = valid_block->CreateTransaction();
    valid_tx->AddInput(chain->GetBlockAt(0)->GetCoinbaseTransaction()->GetHash(), 25);
    valid_tx->AddInput(chain->GetBlockAt(0)->GetCoinbaseTransaction()->GetHash(), 10);
    valid_tx->AddOutput(user2.GetUserId(), "TestToken25");
    valid_tx->AddOutput(user2.GetUserId(), "TestToken10");

    ASSERT_TRUE(chain->Append(valid_block));

    std::cout << "<HEAD>" << std::endl;
    std::cout << (*chain->GetHead()) << std::endl;
    std::cout << (*chain->GetHeadUnclaimedTransactionPool()) << std::endl;
}

int
main(int argc, char** argv){
    testing::InitGoogleTest(&argc, argv);
    std::cout << Token::GetVersion() << std::endl;
    return RUN_ALL_TESTS();
}