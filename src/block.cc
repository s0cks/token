#include "block.h"
#include "server.h"
#include "blockchain.h"
#include "utils/crash_report.h"

namespace Token{
//######################################################################################################################
//                                          Block Header
//######################################################################################################################
    BlockHeader::BlockHeader(Block* blk):
        timestamp_(blk->timestamp_),
        height_(blk->height_),
        previous_hash_(blk->previous_hash_),
        merkle_root_(blk->GetMerkleRoot()),
        hash_(blk->GetHash()),
        bloom_(blk->tx_bloom_){}

    BlockHeader::BlockHeader(Buffer* buff):
        timestamp_(buff->GetLong()),
        height_(buff->GetLong()),
        previous_hash_(buff->GetHash()),
        merkle_root_(buff->GetHash()),
        hash_(buff->GetHash()),
        bloom_(){
    }

    BlockPtr BlockHeader::GetData() const{
        return BlockChain::GetBlock(GetHash());
    }

    bool BlockHeader::Write(Buffer* buff) const{
        buff->PutLong(timestamp_);
        buff->PutLong(height_);
        buff->PutHash(previous_hash_);
        buff->PutHash(merkle_root_);
        buff->PutHash(hash_);
        return true;
    }

//######################################################################################################################
//                                          Block
//######################################################################################################################
    BlockPtr Block::Genesis(){
        int64_t idx;
        InputList inputs;

        OutputList outputs_a;
        for(idx = 0; idx < Block::kNumberOfGenesisOutputs; idx++){
            outputs_a.push_back(Output("VenueA", "TestToken"));
        }

        OutputList outputs_b;
        for(idx = 0; idx < Block::kNumberOfGenesisOutputs; idx++){
            outputs_b.push_back(Output("VenueB", "TestToken"));
        }

        OutputList outputs_c;
        for(idx = 0; idx < Block::kNumberOfGenesisOutputs; idx++){
            outputs_c.push_back(Output("VenueC", "TestToken"));
        }

        TransactionList transactions = {
            std::make_shared<Transaction>(1, inputs, outputs_a, 0),
            std::make_shared<Transaction>(2, inputs, outputs_b, 0),
            std::make_shared<Transaction>(3, inputs, outputs_c, 0),
        };
        return std::make_shared<Block>(0, Hash(), transactions, 0);
    }

    bool Block::ToJson(Json::Value& value) const{
        value["timestamp"] = GetTimestamp();
        value["height"] = GetHeight();
        value["previous_hash"] = GetPreviousHash().HexString();
        value["merkle_root"] = GetMerkleRoot().HexString();
        value["hash"] = GetHash().HexString();
        return true;
    }

    bool Block::Accept(BlockVisitor* vis) const{
        if(!vis->VisitStart())
            return false;
        int64_t idx;
        for(idx = 0;
            idx < GetNumberOfTransactions();
            idx++){

            TransactionPtr tx = transactions_[idx];
            if(!vis->Visit(tx))
                return false;
        }

        return vis->VisitEnd();
    }

    bool Block::Contains(const Hash& hash) const{
        return tx_bloom_.Contains(hash);
    }

    Hash Block::GetMerkleRoot() const{
        MerkleTreeBuilder builder;
        if(!Accept(&builder))
            return Hash();
        std::shared_ptr<MerkleTree> tree = builder.Build();
        return tree->GetRootHash();
    }
}