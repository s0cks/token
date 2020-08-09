#include "block.h"
#include "bytes.h"
#include "server.h"
#include "block_chain.h"
#include "crash_report.h"

namespace Token{
//######################################################################################################################
//                                          Block Header
//######################################################################################################################
    BlockHeader::BlockHeader(Block* blk): BlockHeader(blk->GetTimestamp(), blk->GetHeight(), blk->GetPreviousHash(), blk->GetMerkleRoot(),
                                                      blk->GetHash()){}

    Block* BlockHeader::GetData() const{
        return BlockChain::GetBlockData(GetHash());
    }

//######################################################################################################################
//                                          Block
//######################################################################################################################
    Handle<Block> Block::Genesis(){
        Input* cb_inputs[0];
        Output* cb_outputs_a[Block::kNumberOfGenesisOutputs];
        for(size_t idx = 0; idx < Block::kNumberOfGenesisOutputs; idx++){
            std::string user = "VenueA";
            std::string token = "TestToken";
            cb_outputs_a[idx] = Output::NewInstance(user, token);
        }

        Output* cb_outputs_b[Block::kNumberOfGenesisOutputs];
        for(size_t idx = 0; idx < Block::kNumberOfGenesisOutputs; idx++){
            std::string user = "VenueB";
            std::string token = "TestToken";
            cb_outputs_b[idx] = Output::NewInstance(user, token);
        }

        Output* cb_outputs_c[Block::kNumberOfGenesisOutputs];
        for(size_t idx = 0; idx < Block::kNumberOfGenesisOutputs; idx++){
            std::string user = "VenueC";
            std::string token = "TestToken";
            cb_outputs_c[idx] = Output::NewInstance(user, token);
        }

        Transaction* txs[3] = {
            Transaction::NewInstance(0, cb_inputs, 0, cb_outputs_a, Block::kNumberOfGenesisOutputs, 0),
            Transaction::NewInstance(1, cb_inputs, 0, cb_outputs_b, Block::kNumberOfGenesisOutputs, 0),
            Transaction::NewInstance(2, cb_inputs, 0, cb_outputs_c, Block::kNumberOfGenesisOutputs, 0),
        };
        return NewInstance(0, uint256_t(), txs, 3, 0);
    }

    Handle<Block> Block::NewInstance(uint32_t height, const uint256_t& phash, Transaction** txs, size_t num_txs, uint32_t timestamp){
        return new Block(timestamp, height, phash, txs, num_txs);
    }

    Handle<Block> Block::NewInstance(const BlockHeader& previous, Transaction** txs, size_t num_txs, uint32_t timestamp){
        return new Block(timestamp, previous.GetHeight() + 1, previous.GetHash(), txs, num_txs);
    }

    Handle<Block> Block::NewInstance(std::fstream& fd, uint32_t size){
        uint8_t bytes[size];
        fd.read((char*)bytes, size);
        return NewInstance(bytes);
    }

    Handle<Block> Block::NewInstance(uint8_t* bytes){
        size_t offset = 0;
        int32_t timestamp = DecodeInt(&bytes[offset]);
        offset += 4;

        int32_t height = DecodeInt(&bytes[offset]);
        offset += 4;

        uint256_t phash = DecodeHash(&bytes[offset]);
        offset += uint256_t::kSize;

        int32_t num_txs = DecodeInt(&bytes[offset]);
        offset += 4;

        Transaction* txs[num_txs];
        for(uint32_t idx = 0; idx < num_txs; idx++){
            Transaction* tx = txs[idx] = Transaction::NewInstance(&bytes[offset]);
            offset += tx->GetBufferSize();
        }
        return new Block(timestamp, height, phash, txs, num_txs);
    }

    size_t Block::GetBufferSize() const{
        size_t size = 0;
        size += 4; // timestamp_
        size += 4; // height_
        size += uint256_t::kSize; // previous_hash_
        size += (4 + GetTransactionsLength());
        return size;
    }

    std::string Block::ToString() const{
        std::stringstream stream;
        stream << "Block(#" << GetHeight() << ":" << GetHash() << ")";
        return stream.str();
    }

    bool Block::Encode(uint8_t* bytes) const{
        size_t offset = 0;
        EncodeInt(&bytes[offset], timestamp_);
        offset += 4;

        EncodeInt(&bytes[offset], height_);
        offset += 4;

        EncodeHash(&bytes[offset], previous_hash_);
        offset += uint256_t::kSize;

        EncodeInt(&bytes[offset], num_transactions_);
        offset += 4;
        for(uint32_t idx = 0; idx < num_transactions_; idx++){
            Transaction* tx = transactions_[idx];
            if(!tx->Encode(&bytes[offset])) return false;
            offset += tx->GetBufferSize();
        }
        return true;
    }

    bool Block::Accept(BlockVisitor* vis) const{
        if(!vis->VisitStart()) return false;
        for(size_t idx = 0;
            idx < GetNumberOfTransactions();
            idx++){
            Handle<Transaction> tx = GetTransaction(idx);
            if(!vis->Visit(tx)) return false;
        }
        return vis->VisitEnd();
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

        bool Visit(const Handle<Transaction>& tx){
            return AddLeaf(tx->GetHash());
        }

        bool BuildTree(){
            if(HasTree()) return false;
            if(!GetBlock()->Accept(this)) return false;
            return CreateTree();
        }
    };

    uint256_t Block::GetMerkleRoot() const{
        BlockMerkleTreeBuilder builder(this);
        if(!builder.BuildTree()) return uint256_t();
        MerkleTree* tree = builder.GetTree();
        return tree->GetMerkleRootHash();
    }
}