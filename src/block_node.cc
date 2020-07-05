#include "block_node.h"
#include "allocator.h"
#include "alloc/raw_object.h"

namespace Token{
    BlockNode::BlockNode(const BlockHeader& block):
        previous_(nullptr),
        next_(nullptr),
        block_(block){}

    BlockNode* BlockNode::NewInstance(const BlockHeader& block){
        BlockNode* instance = (BlockNode*)Allocator::Allocate(sizeof(BlockNode), Object::kBlockNode);
        new (instance)BlockNode(block);
        return instance;
    }

    std::string BlockNode::ToString() const{
        std::stringstream ss;
        ss << "BlockNode(" << GetBlock() << ")";
        return ss.str();
    }
}