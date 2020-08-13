#include "block.h"
#include "bytes.h"
#include "server.h"
#include "block_chain.h"
#include "crash_report.h"

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

    BlockHeader::BlockHeader(ByteBuffer* bytes):
        timestamp_(bytes->GetLong()),
        height_(bytes->GetLong()),
        previous_hash_(bytes->GetHash()),
        merkle_root_(bytes->GetHash()),
        hash_(bytes->GetHash()),
        bloom_(){} //TODO: decode(bloom_)

    Block* BlockHeader::GetData() const{
        return BlockChain::GetBlockData(GetHash());
    }

    bool BlockHeader::Encode(ByteBuffer* bytes) const{
        bytes->PutLong(timestamp_);
        bytes->PutLong(height_);
        bytes->PutHash(previous_hash_);
        bytes->PutHash(merkle_root_);
        bytes->PutHash(hash_);
        //TODO: encode(bloom_)
        return true;
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

    Handle<Block> Block::NewInstance(std::fstream& fd, size_t size){
        ByteBuffer bytes(size);
        fd.read((char*)bytes.data(), size);
        return NewInstance(&bytes);
    }

    Handle<Block> Block::NewInstance(ByteBuffer* bytes){
        uint64_t timestamp = bytes->GetLong();
        uint32_t height = bytes->GetInt();
        uint256_t phash = bytes->GetHash();
        uint32_t num_txs = bytes->GetInt();
        Transaction* txs[num_txs];
        for(uint32_t idx = 0; idx < num_txs; idx++)
            txs[idx] = Transaction::NewInstance(bytes);
        return new Block(timestamp, height, phash, txs, num_txs);
    }

    size_t Block::GetBufferSize() const{
        size_t size = 0;
        size += sizeof(uint64_t); // timestamp_
        size += sizeof(uint32_t); // height_
        size += uint256_t::kSize; // previous_hash_
        size += sizeof(uint32_t); // num_transactions_;
        for(uint32_t idx = 0; idx < num_transactions_; idx++)
            size += transactions_[idx]->GetBufferSize(); // transactions_[idx]
        return size;
    }

    std::string Block::ToString() const{
        std::stringstream stream;
        stream << "Block(#" << GetHeight() << ":" << GetHash() << ")";
        return stream.str();
    }

    bool Block::Encode(ByteBuffer* bytes) const{
        bytes->PutLong(timestamp_);
        bytes->PutInt(height_);
        bytes->PutHash(previous_hash_);
        bytes->PutInt(num_transactions_);
        for(uint32_t idx = 0; idx < num_transactions_; idx++)
            if(!transactions_[idx]->Encode(bytes)) return false;
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