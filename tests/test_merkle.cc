#include "test_suite.h"
#include "merkle.h"

namespace Token{
    TEST(TestMerkle, test_tree){
        BlockPtr genesis = Block::Genesis();
        ASSERT_EQ(genesis->GetMerkleRoot(), Hash::FromHexString("2EC98E3D40CB94A7CC6B49A207D628DE47463BA67CF76EAC6AE1ECAF0FC12B77"));
    }

    static inline TransactionPtr
    CreateTransaction(int64_t index, int64_t num_outputs, Timestamp timestamp=GetCurrentTimestamp()){
        InputList inputs;
        OutputList outputs;
        for(int64_t idx = 0; idx < num_outputs; idx++){
            std::stringstream ss;
            ss << "TestToken" << idx;
            outputs.push_back(Output("TestUser", ss.str()));
        }
        return std::make_shared<Transaction>(index, inputs, outputs, timestamp);
    }

    TEST(TestMerkle, test_audit_proof){
        BlockPtr blk1 = Block::Genesis();
        MerkleTreePtr tree1 = MerkleTreeBuilder::Build(blk1);

        Hash h1 = blk1->transactions()[0]->GetHash();
        MerkleProof p1;
        ASSERT_TRUE(tree1->BuildAuditProof(h1, p1));
        ASSERT_TRUE(tree1->VerifyAuditProof(h1, p1));

        TransactionPtr tx2 = CreateTransaction(0, 1);
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
        ASSERT_EQ(tree1->GetRootHash(), Hash::FromHexString("2EC98E3D40CB94A7CC6B49A207D628DE47463BA67CF76EAC6AE1ECAF0FC12B77"));

        TransactionPtr tx = CreateTransaction(0, 1);
        BlockPtr blk1 = BlockPtr(new Block(genesis, { tx }));
        MerkleTreePtr tree2 = MerkleTreeBuilder::Build(blk1);
        tree1->Append(tree2);

        LOG(INFO) << "Merkle Tree #3 (Leaves) " << tree1->GetRootHash() << ":";
        MerkleTreeLogger logger;
        ASSERT_TRUE(tree1->VisitLeaves(&logger));

        Hash h1 = tx->GetHash();
        LOG(INFO) << "auditing for: " << h1;
        MerkleProof p1;
        ASSERT_TRUE(tree1->BuildAuditProof(h1, p1));
        ASSERT_TRUE(tree1->VerifyAuditProof(h1, p1));
    }

    TEST(TestMerkle, test_consistency_proof){
        MerkleTreeLogger logger;

        TransactionPtr tx1 = CreateTransaction(0, 1);
        TransactionPtr tx2 = CreateTransaction(1, 1);

        std::vector<Hash> tree1_data = {
            tx1->GetHash(),
            tx2->GetHash(),
        };
        MerkleTreePtr tree1 = MerkleTreePtr(new MerkleTree(tree1_data));

        LOG(INFO) << "Merkle Tree #1 (" << tree1->GetNumberOfLeaves() << " Leaves) " << tree1->GetRootHash() << ":";
        ASSERT_TRUE(tree1->VisitLeaves(&logger));

        Hash old_root = tree1->GetRootHash();

        TransactionPtr tx3 = CreateTransaction(2, 1);
        TransactionPtr tx4 = CreateTransaction(3, 1);
        std::vector<Hash> tree2_data = {
            tx3->GetHash(),
            tx4->GetHash(),
        };
        MerkleTreePtr tree2 = MerkleTreePtr(new MerkleTree(tree2_data));
        tree1->Append(tree2);

        LOG(INFO) << "Merkle Tree #1 (" << tree1->GetNumberOfLeaves() << " Leaves) " << tree1->GetRootHash() << ":";
        ASSERT_TRUE(tree1->VisitLeaves(&logger));

        MerkleProof proof;
        ASSERT_TRUE(tree1->BuildConsistencyProof(2, proof));
        ASSERT_TRUE(tree1->VerifyConsistencyProof(old_root, proof));
    }

    TEST(TestMerkle, test_consistency_proof_2){
        MerkleTreeLogger logger;

        TransactionPtr tx1 = CreateTransaction(0, 1);
        TransactionPtr tx2 = CreateTransaction(1, 1);

        std::vector<Hash> tree1_data = {
            tx1->GetHash(),
            tx2->GetHash(),
        };
        MerkleTreePtr tree1 = MerkleTreePtr(new MerkleTree(tree1_data));

        Hash r1 = tree1->GetRootHash();

        LOG(INFO) << "Merkle Tree #1 (" << tree1->GetNumberOfLeaves() << " Leaves) " << tree1->GetRootHash() << ":";
        ASSERT_TRUE(tree1->VisitLeaves(&logger));

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

        LOG(INFO) << "Merkle Tree #1 (" << tree1->GetNumberOfLeaves() << " Leaves) " << tree1->GetRootHash() << ":";
        ASSERT_TRUE(tree1->VisitLeaves(&logger));

        MerkleProof p1;
        ASSERT_TRUE(tree1->BuildConsistencyProof(2, p1));
        ASSERT_TRUE(tree1->VerifyConsistencyProof(r1, p1));

        MerkleProof p2;
        ASSERT_TRUE(tree1->BuildConsistencyProof(4, p2));
        ASSERT_TRUE(tree1->VerifyConsistencyProof(r2, p2));
    }
}