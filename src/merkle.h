#ifndef TOKEN_MERKLE_H
#define TOKEN_MERKLE_H

#include "common.h"
#include <vector>
#include <array>

namespace Token{
    typedef CryptoPP::SHA256 HashFunction;
    typedef std::array<Byte, CryptoPP::SHA256::DIGESTSIZE> HashArray;

    class MerkleNode{
    private:
        MerkleNode* parent_;
        MerkleNode* left_;
        MerkleNode* right_;
        HashArray hash_;
    public:
        MerkleNode(HashArray hash):
            parent_(nullptr),
            left_(nullptr),
            right_(nullptr),
            hash_(hash){}
        ~MerkleNode(){
            if(GetLeft()) delete left_;
            if(GetRight()) delete right_;
        }

        MerkleNode* GetParent() const{
            return parent_;
        }

        void SetParent(MerkleNode* node){
            parent_ = node;
        }

        MerkleNode* GetLeft() const{
            return left_;
        }

        void SetLeft(MerkleNode* node){
            left_ = node;
        }

        MerkleNode* GetRight() const{
            return right_;
        }

        void SetRight(MerkleNode* node){
            right_ = node;
        }

        HashArray GetHash() const{
            return hash_;
        }

        bool IsLeaf() const{
            return !GetLeft() && !GetRight();
        }

        void GetHashesAt(size_t level, std::vector<HashArray>& stack) const;
        bool FindItem(const HashArray& hash, std::vector<HashArray>& stack) const;
    };

    class MerkleNodeItem{
    protected:
        MerkleNodeItem(){}
    public:
        virtual ~MerkleNodeItem(){}

        virtual Byte* GetData() const = 0;
        virtual size_t GetDataSize() const = 0;
    };

    class MerkleTree{
    private:
        MerkleNode* head_;
    public:
        MerkleTree(std::vector<MerkleNodeItem*> items);
        ~MerkleTree(){}

        MerkleNode* GetHead() const{
            return head_;
        }

        std::string GetMerkleRoot() const;

        bool ItemExists(MerkleNodeItem* item);

        std::vector<HashArray> GetMerklePath(MerkleNodeItem* item);
        std::vector<HashArray> GetHashesAt(size_t level);
    };
}

#endif //TOKEN_MERKLE_H