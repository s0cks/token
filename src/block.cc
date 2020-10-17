#include "block.h"
#include "byte_buffer.h"
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
        bloom_(){}

    Handle<Block> BlockHeader::GetData() const{
        return BlockChain::GetBlock(GetHash());
    }

    bool BlockHeader::Encode(ByteBuffer* bytes) const{
        bytes->PutLong(timestamp_);
        bytes->PutLong(height_);
        bytes->PutHash(previous_hash_);
        bytes->PutHash(merkle_root_);
        bytes->PutHash(hash_);
        return true;
    }

//######################################################################################################################
//                                          Block
//######################################################################################################################
    Handle<Block> Block::Genesis(){
        Input* inputs[0];
        Output* outputs_a[Block::kNumberOfGenesisOutputs];
        for(size_t idx = 0; idx < Block::kNumberOfGenesisOutputs; idx++){
            std::string user = "VenueA";
            std::string token = "TestToken";
            outputs_a[idx] = Output::NewInstance(user, token);
        }

        Output* outputs_b[Block::kNumberOfGenesisOutputs];
        for(size_t idx = 0; idx < Block::kNumberOfGenesisOutputs; idx++){
            std::string user = "VenueB";
            std::string token = "TestToken";
            outputs_b[idx] = Output::NewInstance(user, token);
        }

        Output* outputs_c[Block::kNumberOfGenesisOutputs];
        for(size_t idx = 0; idx < Block::kNumberOfGenesisOutputs; idx++){
            std::string user = "VenueC";
            std::string token = "TestToken";
            outputs_c[idx] = Output::NewInstance(user, token);
        }

        Transaction* transactions[3] = {
            Transaction::NewInstance(0, inputs, 0, outputs_a, Block::kNumberOfGenesisOutputs, 0),
            Transaction::NewInstance(1, inputs, 0, outputs_b, Block::kNumberOfGenesisOutputs, 0),
            Transaction::NewInstance(2, inputs, 0, outputs_c, Block::kNumberOfGenesisOutputs, 0),
        };
        return NewInstance(0, uint256_t::Null(), transactions, 3, 0);
    }

    Handle<Block> Block::NewInstance(std::fstream& fd, size_t size){
        ByteBuffer bytes(size);
        fd.read((char*)bytes.data(), size);
        return NewInstance(&bytes);
    }

    Handle<Block> Block::NewInstance(ByteBuffer* bytes){
        Timestamp timestamp = bytes->GetLong();
        intptr_t height = bytes->GetLong();
        uint256_t phash = bytes->GetHash();
        intptr_t num_txs = bytes->GetLong();

        LOG(INFO) << "reading " << num_txs << " transactions for block";

        Transaction* transactions[num_txs];
        for(uint32_t idx = 0; idx < num_txs; idx++)
            transactions[idx] = Transaction::NewInstance(bytes);
        return new Block(timestamp, height, phash, transactions, num_txs);
    }

    std::string Block::ToString() const{
        std::stringstream stream;
        stream << "Block(" << GetHeight() << ":" << GetHash() << "; " << GetNumberOfTransactions() << " Transactions)";
        return stream.str();
    }

    bool Block::Accept(BlockVisitor* vis) const{
        if(!vis->VisitStart()) return false;
        for(intptr_t idx = 0;
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
        if(!builder.BuildTree()) return uint256_t::Null();
        MerkleTree* tree = builder.GetTree();
        return tree->GetMerkleRootHash();
    }
}