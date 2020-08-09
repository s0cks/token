#include "block_node.h"
#include "allocator.h"
#include "block_chain.h"

namespace Token{
    BlockNode::BlockNode(const BlockHeader& block):
        previous_(nullptr),
        next_(nullptr),
        block_(block){}

    BlockNode* BlockNode::NewInstance(const BlockHeader& block){
        return new BlockNode(block);
    }

    Handle<Block> BlockNode::GetData() const{
        return BlockChain::GetBlockData(GetHash());
    }

    std::string BlockNode::ToString() const{
        std::stringstream ss;
        ss << "BlockNode(" << GetBlock() << ")";
        return ss.str();
    }
}