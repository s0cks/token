#ifndef TOKEN_BLOCKCHAIN_H
#define TOKEN_BLOCKCHAIN_H

#include <map>
#include "common.h"
#include "array.h"
#include "bytes.h"
#include "block.h"
#include "user.h"
#include "utxo.h"

namespace Token{
    class BlockChainNode{
    private:
        Block* block_;
        BlockChainNode* parent_;
        Array<BlockChainNode*> children_;
        unsigned int height_;

        inline void
        AddChild(BlockChainNode* node){
            children_.Add(node);
        }

        friend class BlockChain;
    public:
        BlockChainNode(BlockChainNode* parent, Block* block):
            parent_(parent),
            block_(block),
            height_(0),
            children_(0xA){
            if(parent_ != nullptr){
                height_ = parent->GetHeight() + 1;
                parent->AddChild(this);
            }
        }

        Block* GetBlock() const{
            return block_;
        }

        Array<BlockChainNode*> GetChildren() const{
            return children_;
        }

        unsigned int GetHeight() const{
            return block_->GetHeight();
        }
    };

    class BlockChain{
    private:
        Array<BlockChainNode*>* heads_;
        BlockChainNode* head_;
        std::map<std::string, BlockChainNode*> nodes_;
        unsigned int height_;
        TransactionPool tx_pool_;
        std::string root_;

        bool AppendGenesis(Block* block);

        inline BlockChainNode*
        GetBlockParent(Block* block){
            std::string target = block->GetPreviousHash();
            auto found = nodes_.find(target);
            if(found == nodes_.end()) return nullptr;
            return found->second;
        }

        inline std::string
        GetLedgerRoot(){
            std::stringstream stream;
            stream << GetRootFileSystem() << "/ledger";
            return stream.str();
        }

        std::string GetLedgerFile(const std::string& filename){
            std::stringstream stream;
            stream << GetLedgerRoot() << filename;
            return stream.str();
        }

        std::string GetUnclaimedTransactionDatabase(){
            std::stringstream stream;
            stream << GetRootFileSystem() << "/unclaimed.db";
            return stream.str();
        }

        BlockChain():
            heads_(new Array<BlockChainNode*>(0xA)),
            root_(),
            tx_pool_(),
            nodes_(),
            height_(0){}
    public:
        class Server;

        static BlockChain* GetInstance();
        static bool Initialize(const std::string& path, Block* genesis);
        static Server* GetServerInstance();

        Block* GetBlockFromHash(const std::string& hash) const{
            return nodes_.find(hash)->second->GetBlock();
        }

        Block* GetHead() const{
            return head_->GetBlock();
        }

        Block* GetBlockAt(int height) const{
            if(height > GetHeight()) return nullptr;
            Block* head = GetHead();
            while(head && head->GetHeight() > height) head = GetBlockFromHash(head->GetPreviousHash());
            return head;
        }

        unsigned int GetHeight() const{
            return GetHead()->GetHeight();
        }

        TransactionPool* GetTransactionPool(){
            return &tx_pool_;
        }

        void AddTransaction(Transaction* tx){
            return GetTransactionPool()->AddTransaction(tx);
        }

        //TODO: Remove this
        void SetHead(Block* block){
            nodes_.clear();
            heads_->Clear();
            height_ = 0;
            AppendGenesis(block);
        }

        void SetRoot(const std::string& root){
            root_ = root;
        }

        std::string GetRootFileSystem(){
            return root_;
        }

        bool HasRoot(){
            return !GetRootFileSystem().empty();
        }

        Block* CreateBlock();
        bool Append(Block* block);
        bool Load(const std::string& root);
        bool Load(const std::string& root, const std::string& addr, int port);
        bool Save(const std::string& root);
        bool Save();
    };
}

#endif //TOKEN_BLOCKCHAIN_H