#include "block.h"
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

    BlockHeader::BlockHeader(const Handle<Buffer>& buff):
        timestamp_(buff->GetLong()),
        height_(buff->GetLong()),
        previous_hash_(buff->GetHash()),
        merkle_root_(buff->GetHash()),
        hash_(buff->GetHash()),
        bloom_(){
    }

    Handle<Block> BlockHeader::GetData() const{
        return BlockChain::GetBlock(GetHash());
    }

    bool BlockHeader::Write(const Handle<Buffer>& buff) const{
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
    Handle<Block> Block::Genesis(){
        Input* inputs[0];
        Output* outputs_a[Block::kNumberOfGenesisOutputs];
        for(size_t idx = 0; idx < Block::kNumberOfGenesisOutputs; idx++){
            std::string user = "VenueA";
            std::stringstream ss;
            ss << "TestToken" << idx;
            outputs_a[idx] = Output::NewInstance(user, ss.str());
        }

        Output* outputs_b[Block::kNumberOfGenesisOutputs];
        for(size_t idx = 0; idx < Block::kNumberOfGenesisOutputs; idx++){
            std::string user = "VenueB";
            std::stringstream ss;
            ss << "TestToken" << idx;
            outputs_b[idx] = Output::NewInstance(user, ss.str());
        }

        Output* outputs_c[Block::kNumberOfGenesisOutputs];
        for(size_t idx = 0; idx < Block::kNumberOfGenesisOutputs; idx++){
            std::string user = "VenueC";
            std::stringstream ss;
            ss << "TestToken" << idx;
            outputs_c[idx] = Output::NewInstance(user, ss.str());
        }

        Transaction* transactions[3] = {
            Transaction::NewInstance(0, inputs, 0, outputs_a, Block::kNumberOfGenesisOutputs, 0),
            Transaction::NewInstance(1, inputs, 0, outputs_b, Block::kNumberOfGenesisOutputs, 0),
            Transaction::NewInstance(2, inputs, 0, outputs_c, Block::kNumberOfGenesisOutputs, 0),
        };
        return NewInstance(0, Hash(), transactions, 3, 0);
    }

    Handle<Block> Block::NewInstance(std::fstream& fd, size_t size){
        Handle<Buffer> buff = Buffer::NewInstance(size);
        buff->ReadBytesFrom(fd, size);
        return NewInstance(buff);
    }

    Handle<Block> Block::NewInstance(const Handle<Buffer>& buff){
        Timestamp timestamp = buff->GetLong();
        intptr_t height = buff->GetLong();
        Hash phash = buff->GetHash();
        intptr_t num_txs = buff->GetLong();

        Transaction* transactions[num_txs];
        for(intptr_t idx = 0; idx < num_txs; idx++){
            transactions[idx] = Transaction::NewInstance(buff);
        }
        return new Block(timestamp, height, phash, transactions, num_txs);
    }

    bool Block::Write(const Handle<Buffer>& buff) const{
        buff->PutLong(timestamp_);
        buff->PutLong(height_);
        buff->PutHash(previous_hash_);
        buff->PutLong(num_transactions_);
        for(intptr_t idx = 0;
            idx < num_transactions_;
            idx++){
            transactions_[idx]->Write(buff);
        }
        return true;
    }

    bool Block::Compare(const Handle<Block>& b) const{
        return GetHeight() < b->GetHeight();
    }

    std::string Block::ToString() const{
        std::stringstream stream;
        stream << "Block(#" << GetHeight() << ", " << GetNumberOfTransactions() << " Transactions)";
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

    bool Block::Contains(const Hash& hash) const{
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

    Hash Block::GetMerkleRoot() const{
        BlockMerkleTreeBuilder builder(this);
        if(!builder.BuildTree()) return Hash();
        MerkleTree* tree = builder.GetTree();
        return tree->GetMerkleRootHash();
    }
}