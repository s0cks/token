#include "block_node.h"
#include "allocator.h"

namespace Token{
    BlockNode::BlockNode(const BlockHeader& block):
        previous_(nullptr),
        next_(nullptr),
        block_(block){}

    BlockNode* BlockNode::NewInstance(const BlockHeader& block){
        return new BlockNode(block);
    }

    std::string BlockNode::ToString() const{
        std::stringstream ss;
        ss << "BlockNode(" << GetBlock() << ")";
        return ss.str();
    }
}