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
        friend class TransactionPool;
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

        int GetPeerCount();
        bool IsPeerRegistered(PeerClient* p);
        std::string GetPeer(int n);
        void SetPeerCount(int n);
        void RegisterPeer(PeerClient* p);
        void DeregisterPeer(PeerClient* p);

        std::string GetBlockLocation(const std::string& hash);
        std::string GetBlockLocation(Block* block);
        std::string GetBlockLocation(uint32_t height);
        bool SetBlockLocation(Block* block, const std::string& filename);
        bool SetHeight(int height);

        static inline leveldb::DB*
        GetState(){
            return GetInstance()->state_;
        }

        static inline std::string
        GetBlockFile(Block* block){
            std::stringstream stream;
            stream << GetRootDirectory() << "/blk" << block->GetHeight() << ".dat";
            return stream.str();
        }

        static Node* GetNodeAt(uint32_t height);

        bool InitializeChainHead();
        bool InitializeChainState(const std::string& root);
        bool SetHead(Block* block);
        bool SetHead(const std::string& hash);

        static bool SaveBlock(Block* block);
        static Block* LoadBlock(uint32_t height);
        static Block* LoadBlock(const std::string& hash);
        Block* CreateBlock(); //TODO: Remove

        BlockChain():
            rwlock_(),
            state_(nullptr),
            head_(nullptr){
            pthread_rwlock_init(&rwlock_, NULL);
        }
    public:
        static BlockChain* GetInstance();

        static std::string GetRootDirectory();
        static void Accept(BlockChainVisitor* vis);
        static Block* GetHead();
        static Block* GetGenesis();
        static Block* GetBlock(const std::string& hash);
        static Block* GetBlock(uint32_t height);
        static uint32_t GetHeight();
        static bool HasHead();
        static bool ContainsBlock(const std::string& hash);
        static bool ContainsBlock(Block* block);
        static bool AppendBlock(Block* block);
        static bool Initialize(const std::string& path);
    };
}

#endif //TOKEN_BLOCKCHAIN_H