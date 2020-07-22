#include "block.h"
#include "block_chain.h"
#include "server.h"
#include "crash_report.h"

namespace Token{
//######################################################################################################################
//                                          Block Header
//######################################################################################################################
    BlockHeader::BlockHeader(Block* blk): BlockHeader(blk->GetTimestamp(), blk->GetHeight(), blk->GetPreviousHash(), blk->GetMerkleRoot(),
                                                      blk->GetSHA256Hash()){}

    Block* BlockHeader::GetData() const{
        return BlockChain::GetBlockData(GetHash());
    }

//######################################################################################################################
//                                          Block
//######################################################################################################################
    Handle<Block> Block::Genesis(){
        Transaction::InputList cb_inputs = {};
        Transaction::OutputList cb_outputs;
        for(size_t idx = 0; idx < BlockChain::kNumberOfGenesisOutputs; idx++){
            std::string user = "TestUser";
            std::stringstream token;
            token << "TestToken" << idx;
            cb_outputs.push_back(Output("TestUser", token.str()));
        }

        Handle<Array<Transaction>> txs = Array<Transaction>::New(1);
        txs->Put(0, Transaction::NewInstance(0, cb_inputs, cb_outputs, 0));
        return NewInstance(0, uint256_t(), txs, 0);
    }

    Handle<Block> Block::NewInstance(uint32_t height, const Token::uint256_t &phash, const Handle<Array<Transaction>>& txs, uint32_t timestamp){
        return new Block(timestamp, height, phash, txs);
    }

    Handle<Block> Block::NewInstance(const BlockHeader &parent, const Handle<Array<Transaction>>& txs, uint32_t timestamp){
        return NewInstance(parent.GetHeight() + 1, parent.GetHash(), txs, timestamp);
    }

    Handle<Block> Block::NewInstance(const RawBlock& raw){
        size_t num_txs = raw.transactions_size();
        Handle<Array<Transaction>> txs = Array<Transaction>::New(num_txs);
        for(size_t idx = 0; idx < num_txs; idx++) txs->Put(idx, Transaction::NewInstance(raw.transactions(idx)));
        return NewInstance(raw.height(), HashFromHexString(raw.previous_hash()), txs, raw.timestamp());
    }

    Handle<Block> Block::NewInstance(std::fstream& fd){
        RawBlock raw;
        if(!raw.ParseFromIstream(&fd)) return nullptr;
        return NewInstance(raw);
    }

    std::string Block::ToString() const{
        std::stringstream stream;
        stream << "Block(#" << GetHeight() << "/" << GetSHA256Hash() << ")";
        return stream.str();
    }

    bool Block::WriteToMessage(RawBlock& raw) const{
        raw.set_timestamp(timestamp_);
        raw.set_height(height_);
        raw.set_previous_hash(HexString(previous_hash_));
        raw.set_merkle_root(HexString(GetMerkleRoot()));
        for(uint32_t idx = 0; idx < GetNumberOfTransactions(); idx++){
            RawTransaction* raw_tx = raw.add_transactions();
            Handle<Transaction> tx = GetTransaction(idx);
            tx->WriteToMessage((*raw_tx));
        }
        return true;
    }

    bool Block::Accept(BlockVisitor* vis) const{
        if(!vis->VisitStart()) return false;
        for(size_t idx = 0;
            idx < GetNumberOfTransactions();
            idx++){
            Transaction* tx = GetTransaction(idx);
            if(!vis->Visit(tx)) return false;
        }
        return vis->VisitEnd();
    }

    bool Block::Finalize(){
        return true;
    }

    bool Block::Contains(const uint256_t& hash) const{
        return tx_bloom_.Contains(hash);
    }

    class BlockMerkleTreeBuilder : public BlockVisitor,
                                   public MerkleTreeBuilder{
    private:
        const Block* block_;
    public:
        BlockMerkleTreeBuilder(const Block* block):
                BlockVisitor(),
                MerkleTreeBuilder(),
                block_(block){}
        ~BlockMerkleTreeBuilder(){}

        const Block* GetBlock() const{
            return block_;
        }

        bool Visit(Transaction* tx){
            return AddLeaf(tx->GetSHA256Hash());
        }

        bool BuildTree(){
            if(HasTree()) return false;
            if(!GetBlock()->Accept(this)) return false;
            return CreateTree();
        }
    };

    uint256_t Block::GetMerkleRoot() const{
        //TODO: scope merkle tree
        BlockMerkleTreeBuilder builder(this);
        if(!builder.BuildTree()) return uint256_t();
        MerkleTree* tree = builder.GetTree();
        return tree->GetMerkleRootHash();
    }
}