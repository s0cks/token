#include "merkle.h"
#include "block.h"

namespace Token{
    std::string ConcatHash(const std::string& first, const std::string& second){
        std::string hash = first + ":" + second;
        std::string digest;
        CryptoPP::SHA256 func;
        CryptoPP::StringSource source(hash, true, new CryptoPP::HashFilter(func, new CryptoPP::StringSink(digest)));
        return digest;
    }

    std::string ConstructMerkleTree(size_t height, std::vector<std::string>& leaves){
        if(height == 1 && !leaves.empty()){
            std::string front = leaves.front();
            leaves.erase(leaves.begin());
            return front;
        } else if(height > 1){
            std::string left = ConstructMerkleTree(height - 1, leaves);
            std::string right;
            if(leaves.empty()){
                right = left;
            } else{
                right = ConstructMerkleTree(height - 1, leaves);
            }
            return ConcatHash(left, right);
        }
        return "";
    }

    std::string GetMerkleRoot(std::vector<std::string>& leaves){
        size_t height = std::ceil(log2(leaves.size())) + 1;
        return ConstructMerkleTree(height, leaves);
    }

    std::string GetBlockMerkleRoot(Block* block){
        std::vector<std::string> transactions;
        for(size_t idx = 0; idx < block->GetNumberOfTransactions(); idx++){
            Transaction* tx = block->GetTransactionAt(idx);
            transactions.push_back(tx->GetHash());
        }
        return GetMerkleRoot(transactions);
    }
}