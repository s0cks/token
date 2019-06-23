#include "merkle.h"
#include <sstream>
#include <cmath>

namespace Token{
    static const size_t DIGEST_SIZE = CryptoPP::SHA256::DIGESTSIZE;

    static HashArray ConcatHash(const HashArray& first, const HashArray& second){
        const size_t source_len = DIGEST_SIZE * 2;
        std::array<Byte, source_len> source;
        std::copy(first.begin(), first.end(), source.begin());
        std::copy(second.begin(), second.end(), source.begin() + DIGEST_SIZE);
        HashArray digest;
        HashFunction func;
        CryptoPP::ArraySource src(source.data(), source_len, true, new CryptoPP::HashFilter(func, new CryptoPP::ArraySink(digest.data(), DIGEST_SIZE)));
        return digest;
    }

    bool MerkleNode::FindItem(const HashArray& hash, std::vector<HashArray>& stack) const{
        if(GetHash() == hash) return true;
        else if(GetLeft() && GetRight()){
            if(GetLeft()->FindItem(hash, stack)){
                stack.push_back(GetRight()->GetHash());
                return true;
            } else if(GetRight()->GetHash() != GetLeft()->GetHash() && GetRight()->FindItem(hash, stack)){
                stack.push_back(GetLeft()->GetHash());
                return true;
            }
        }
        return false;
    }

    void MerkleNode::GetHashesAt(size_t level, std::vector<HashArray>& stack) const{
        if(level == 0){
            stack.push_back(GetHash());
        } else{
            if(GetLeft()) this->GetLeft()->GetHashesAt(level - 1, stack);
            if(GetRight()) this->GetRight()->GetHashesAt(level - 1, stack);
        }
    }

    static MerkleNode* ConstructMerkleTree(size_t height, std::vector<MerkleNodeItem*>& items){
        if(height == 1 && !items.empty()){
            MerkleNodeItem* item = items.front();
            items.erase(items.begin());
            return new MerkleNode(item->GetHashArray());
        } else if(height > 1){
            MerkleNode* lchild = ConstructMerkleTree(height - 1, items);
            MerkleNode* rchild;
            if(items.empty()){
                rchild = new MerkleNode(lchild->GetHash());
            } else{
                rchild = ConstructMerkleTree(height - 1, items);
            }
            HashArray hash = ConcatHash(lchild->GetHash(), rchild->GetHash());
            MerkleNode* new_node = new MerkleNode(hash);
            new_node->SetLeft(lchild);
            new_node->SetRight(rchild);
            return new_node;
        }
        return nullptr;
    }

    MerkleTree::MerkleTree(std::vector<MerkleNodeItem*> items){
        size_t height = std::ceil(log2(items.size())) + 1;
        head_ = ConstructMerkleTree(height, items);
    }

    std::string MerkleTree::GetMerkleRoot() const{
        std::string result;
        HashArray hash = GetHead()->GetHash();
        CryptoPP::ArraySource source(hash.data(), hash.size(), true, new CryptoPP::HexEncoder(new CryptoPP::StringSink(result)));
        return result;
    }

    bool MerkleTree::ItemExists(MerkleNodeItem* item){
        std::vector<HashArray> stack;
        return GetHead()->FindItem(item->GetHashArray(), stack);
    }

    std::vector<HashArray> MerkleTree::GetMerklePath(MerkleNodeItem* item){
        std::vector<HashArray> stack;
        GetHead()->FindItem(item->GetHashArray(), stack);
        return stack;
    }

    std::vector<HashArray> MerkleTree::GetHashesAt(size_t level){
        std::vector<HashArray> stack;
        GetHead()->GetHashesAt(level, stack);
        return stack;
    }
}