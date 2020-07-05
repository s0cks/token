#include "block_node.h"
#include "allocator.h"

namespace Token{
    BlockNode* BlockNode::NewInstance(const BlockHeader& block){
        BlockNode* instance = (BlockNode*)Allocator::Allocate(sizeof(BlockNode));
        new (instance)BlockNode(block);
        return instance;
    }

    std::string BlockNode::ToString() const{
        std::stringstream ss;
        ss << "BlockNode(" << GetBlock() << ")";
        return ss.str();
    }
}