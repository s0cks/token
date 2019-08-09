#ifndef TOKEN_BLOCKCHAIN_STATE_H
#define TOKEN_BLOCKCHAIN_STATE_H

#include <rapidjson/document.h>
#include "common.h"
#include "block.h"

namespace Token{
    class BlockChainState{
    private:
        std::string root_;
        rapidjson::Document doc_;
        unsigned int height_;

        BlockChainState(const std::string& root):
            height_(-1),
            doc_(rapidjson::kObjectType),
            root_(root){}

        std::string GetStateFile() const;
        std::string GetBlockFile(uint32_t idx) const;
        std::string GetUnclaimedTransactionPoolFile() const;

        void LoadState();
        void InitializeState();
        void SaveState();

        void SetHeight(unsigned int height){
            height_ = height;
            SaveState();
        }

        static bool CanLoadStateFrom(const std::string& root);
        static BlockChainState* LoadState(const std::string& root);

        friend class Block;
        friend class BlockChain;
    public:
        ~BlockChainState(){}

        unsigned int GetHeight() const{
            return height_;
        }

        std::string GetRootDirectory() const{
            return root_;
        }
    };
}

#endif //TOKEN_BLOCKCHAIN_STATE_H