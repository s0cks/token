#ifndef TOKEN_BLOCKCHAIN_H
#define TOKEN_BLOCKCHAIN_H

#include <leveldb/db.h>
#include <map>
#include "common.h"
#include "array.h"
#include "bytes.h"
#include "block.h"
#include "user.h"
#include "utxo.h"
#include "peer.h"

namespace Token{
    class BlockChainVisitor{
    public:
        BlockChainVisitor(){}
        virtual ~BlockChainVisitor() = default;
        virtual bool Visit(Block* block) = 0;
    };

    class BlockChain{
    public:
        class Node{
        private:
            Node* parent_;
            std::vector<Node*> children_;
            Block* block_;

            inline void
            AddChild(Node* child){
                children_.push_back(child);
            }
        public:
            Node(Node* parent, Block* block):
                parent_(parent),
                children_(),
                block_(block){
                if(!IsRoot()) GetParent()->AddChild(this);
            }
            Node(Block* block): Node(nullptr, block){}
            ~Node(){
                for(auto& it : children_){
                    delete it;
                }
            }

            Node* GetParent() const{
                return parent_;
            }

            Block* GetBlock() const{
                return block_;
            }

            bool IsRoot() const{
                return GetParent() == nullptr;
            }

            size_t GetNumberOfChildren() const{
                return children_.size();
            }

            Node* GetChildAt(int idx) const{
                return children_[idx];
            }

            void Accept(BlockChainVisitor* vis);
        };
    private:
        friend class BlockChainServer;
        friend class PeerSession;
        friend class PeerClient;
        friend class PeerBlockResolver;
        friend class LocalBlockResolver;
        friend class BlockChainResolver;

        std::string root_;
        leveldb::DB* state_;
        Node* head_;
        pthread_rwlock_t rwlock_;
        TransactionPool tx_pool_;
        pthread_t server_thread_;

        Node* GetNodeAt(int height) const;

        int GetPeerCount();
        bool IsPeerRegistered(PeerClient* p);
        std::string GetPeer(int n);
        void SetPeerCount(int n);
        void RegisterPeer(PeerClient* p);
        void DeregisterPeer(PeerClient* p);

        void RegisterBlock(const std::string& hash, int height);
        void SetHeight(int height);
        void SetGenesisHash(const std::string& hash);
        bool SetHead(Block* block);
        bool SetHead(const std::string& hash);

        inline leveldb::DB*
        GetState() const{
            return state_;
        }

        inline Node*
        GetHeadNode() const{
            return head_;
        }

        std::string GetGenesisHash() const;
        std::string GetBlockDataFile(int height) const;
        int GetBlockHeightFromHash(const std::string& hash) const;
        Block* LoadBlock(int height) const;
        bool SaveChain();
        bool InitializeChainHead();
        bool InitializeChainState(const std::string& root);

        BlockChain():
            rwlock_(),
            state_(nullptr),
            tx_pool_(),
            head_(nullptr){
            pthread_rwlock_init(&rwlock_, NULL);
        }
    public:
        static BlockChain* GetInstance();

        void Accept(BlockChainVisitor* vis) const;
        Block* GetHead();
        Block* GetGenesis() const;
        Block* GetBlockFromHash(const std::string& hash) const;
        Block* GetBlockAt(int height) const;
        int GetHeight() const;

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
        bool HasBlock(const std::string& hash) const;
        bool Append(Block* block);

        static bool Initialize(const std::string& path);
        static bool Save(Block* block); // TODO: Remove
        static bool GetBlockList(std::vector<std::string>& blocks); // TODO: Refactor
    };
}

#endif //TOKEN_BLOCKCHAIN_H