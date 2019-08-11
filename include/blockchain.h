#ifndef TOKEN_BLOCKCHAIN_H
#define TOKEN_BLOCKCHAIN_H

#include <map>
#include "common.h"
#include "array.h"
#include "bytes.h"
#include "blockchain_state.h"
#include "block.h"
#include "user.h"
#include "utxo.h"
#include "node/node.h"

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
        BlockChainState* state_;
        Array<BlockChainNode*>* heads_;
        BlockChainNode* head_;
        std::map<std::string, BlockChainNode*> nodes_;
        TransactionPool tx_pool_;
        pthread_t server_thread_;
        pthread_rwlock_t rwlock_;

        bool AppendGenesis(Block* block);

        inline BlockChainNode*
        GetBlockParent(Block* block){
            std::string target = block->GetPreviousHash();
            auto found = nodes_.find(target);
            if(found == nodes_.end()) return nullptr;
            return found->second;
        }

        inline bool
        HasState(){
            return state_ != nullptr;
        }

        inline void
        SetState(BlockChainState* state){
            if(HasState()) return;
            state_ = state;
        }

        inline BlockChainState*
        GetState() const{
            return state_;
        }

        inline std::string
        GetBlockDataFile(int idx){
            return GetState()->GetBlockFile(idx);
        }

        BlockChain():
            rwlock_(),
            state_(nullptr),
            heads_(new Array<BlockChainNode*>(0xA)),
            tx_pool_(),
            nodes_(){
            pthread_rwlock_init(&rwlock_, NULL);
        }
    public:
        static BlockChain* GetInstance();
        static bool Initialize(const std::string& path, Block* genesis);
        static BlockChainServer* GetServerInstance();

        Block* GetBlockFromHash(const std::string& hash) const{
            return nodes_.find(hash)->second->GetBlock();
        }

        Block* GetHead();

        Block* GetBlockAt(int height){
            if(height > GetHeight()) return nullptr;
            Block* head = GetHead();
            while(head && head->GetHeight() > height) head = GetBlockFromHash(head->GetPreviousHash());
            return head;
        }

        int GetHeight() const{
            int height = GetState()->GetHeight();
            if(height < 0) return 0;
            return height;
        }

        TransactionPool* GetTransactionPool(){
            return &tx_pool_;
        }

        void AddTransaction(Transaction* tx){
            return GetTransactionPool()->AddTransaction(tx);
        }

        bool HasHead() const{
            return head_ != nullptr;
        }

        Block* CreateBlock();
        bool Append(Block* block);
        bool Load(const std::string& root);
        bool Save();
        void StartServer(int port);
        void StopServer();
        void WaitForServerShutdown();
    };
}

#endif //TOKEN_BLOCKCHAIN_H