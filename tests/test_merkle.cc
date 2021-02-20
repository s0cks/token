#include "test_suite.h"
#include "merkle/tree.h"

namespace token{
    TEST(TestMerkle, test_tree){
        BlockPtr genesis = Block::Genesis();
        ASSERT_EQ(genesis->GetMerkleRoot(), Hash::FromHexString("DE59C00D7898EE129382776FB3E007EBF43E9C99FA74CFDDC2A3EF708AC1E884"));
    }

    static inline IndexedTransactionPtr
    CreateTransaction(int64_t index, int64_t num_outputs, Timestamp timestamp=Clock::now()){
        InputList inputs;
        OutputList outputs;
        for(int64_t idx = 0; idx < num_outputs; idx++){
            std::stringstream ss;
            ss << "TestToken" << idx;
            outputs.push_back(Output("TestUser", ss.str()));
        }
        return std::make_shared<IndexedTransaction>(timestamp, index, inputs, outputs);
    }

    TEST(TestMerkle, test_audit_proof){
        BlockPtr blk1 = Block::Genesis();
        MerkleTreePtr tree1 = MerkleTreeBuilder::Build(blk1);

        Hash h1 = (*blk1->transactions_begin())->GetHash();
        MerkleProof p1;
        ASSERT_TRUE(tree1->BuildAuditProof(h1, p1));
        ASSERT_TRUE(tree1->VerifyAuditProof(h1, p1));

        IndexedTransactionPtr tx2 = CreateTransaction(0, 1);
        Hash h2 = tx2->GetHash();
        MerkleProof p2;
        ASSERT_FALSE(tree1->BuildAuditProof(h2, p2));
        ASSERT_FALSE(tree1->VerifyAuditProof(h2, p2));

        BlockPtr blk2 = BlockPtr(new Block(blk1, { tx2 }));
        MerkleTreePtr tree2 = MerkleTreeBuilder::Build(blk2);

        Hash h3 = tx2->GetHash();
        MerkleProof p3;
        ASSERT_TRUE(tree2->BuildAuditProof(h3, p3));
        ASSERT_TRUE(tree2->VerifyAuditProof(h3, p3));
    }

    TEST(TestMerkle, test_append){
        BlockPtr genesis = Block::Genesis();
        MerkleTreePtr tree1 = MerkleTreeBuilder::Build(genesis);
        ASSERT_EQ(tree1->GetRootHash(), Hash::FromHexString("DE59C00D7898EE129382776FB3E007EBF43E9C99FA74CFDDC2A3EF708AC1E884"));

        IndexedTransactionPtr tx = CreateTransaction(0, 1);
        BlockPtr blk1 = BlockPtr(new Block(genesis, { tx }));
        MerkleTreePtr tree2 = MerkleTreeBuilder::Build(blk1);
        tree1->Append(tree2);

        Hash h1 = tx->GetHash();
        MerkleProof p1;
        ASSERT_TRUE(tree1->BuildAuditProof(h1, p1));
        ASSERT_TRUE(tree1->VerifyAuditProof(h1, p1));
    }

    TEST(TestMerkle, test_consistency_proof){
        TransactionPtr tx1 = CreateTransaction(0, 1);
        TransactionPtr tx2 = CreateTransaction(1, 1);

        std::vector<Hash> tree1_data = {
            tx1->GetHash(),
            tx2->GetHash(),
        };
        MerkleTreePtr tree1 = MerkleTreePtr(new MerkleTree(tree1_data));

        Hash old_root = tree1->GetRootHash();

        TransactionPtr tx3 = CreateTransaction(2, 1);
        TransactionPtr tx4 = CreateTransaction(3, 1);
        std::vector<Hash> tree2_data = {
            tx3->GetHash(),
            tx4->GetHash(),
        };
        MerkleTreePtr tree2 = MerkleTreePtr(new MerkleTree(tree2_data));
        tree1->Append(tree2);

        MerkleProof proof;
        ASSERT_TRUE(tree1->BuildConsistencyProof(2, proof));
        ASSERT_TRUE(tree1->VerifyConsistencyProof(old_root, proof));
    }

    TEST(TestMerkle, test_consistency_proof_2){
        TransactionPtr tx1 = CreateTransaction(0, 1);
        TransactionPtr tx2 = CreateTransaction(1, 1);

        std::vector<Hash> tree1_data = {
            tx1->GetHash(),
            tx2->GetHash(),
        };
        MerkleTreePtr tree1 = MerkleTreePtr(new MerkleTree(tree1_data));

        Hash r1 = tree1->GetRootHash();

        TransactionPtr tx3 = CreateTransaction(2, 1);
        TransactionPtr tx4 = CreateTransaction(3, 1);
        std::vector<Hash> tree2_data = {
            tx3->GetHash(),
            tx4->GetHash(),
        };
        MerkleTreePtr tree2 = MerkleTreePtr(new MerkleTree(tree2_data));
        tree1->Append(tree2);

        Hash r2 = tree1->GetRootHash();

        TransactionPtr tx5 = CreateTransaction(4, 1);
        TransactionPtr tx6 = CreateTransaction(5, 1);
        std::vector<Hash> tree3_data = {
            tx5->GetHash(),
            tx6->GetHash(),
        };

        MerkleProof p1;
        ASSERT_TRUE(tree1->BuildConsistencyProof(2, p1));
        ASSERT_TRUE(tree1->VerifyConsistencyProof(r1, p1));

        MerkleProof p2;
        ASSERT_TRUE(tree1->BuildConsistencyProof(4, p2));
        ASSERT_TRUE(tree1->VerifyConsistencyProof(r2, p2));
    }
}