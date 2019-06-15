#ifndef TOKEN_BLOCKCHAIN_H
#define TOKEN_BLOCKCHAIN_H

#include <uv.h>
#include <memory>
#include <map>
#include "common.h"
#include "array.h"
#include "bytes.h"
#include "message.h"
#include "block.h"
#include "tx_pool.h"
#include "user.h"
#include "utxo.h"
#include "session.h"

namespace Token{
    class GetHeadResponse;

    class BlockChainNode{
    private:
        Block* block_;
        BlockChainNode* parent_;
        Array<BlockChainNode*> children_;
        unsigned int height_;
        UnclaimedTransactionPool utxo_pool_;

        inline void
        AddChild(BlockChainNode* node){
            children_.Add(node);
        }

        friend class BlockChain;
    public:
        BlockChainNode(BlockChainNode* parent, Block* block):
            parent_(parent),
            utxo_pool_(),
            block_(block),
            height_(0),
            children_(0xA){
            if(parent_ != nullptr){
                height_ = parent->GetHeight() + 1;
                parent->AddChild(this);
            }
        }
        BlockChainNode(BlockChainNode* parent, Block* block, UnclaimedTransactionPool utxo_pool):
            parent_(parent),
            utxo_pool_(utxo_pool),
            block_(block),
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

        UnclaimedTransactionPool* GetUnclainedTransactionPool(){
            return &utxo_pool_;
        }
    };

    class BlockChain{
    private:
        Array<BlockChainNode*>* heads_;
        BlockChainNode* head_;
        std::map<std::string, BlockChainNode*> nodes_;
        unsigned int height_;
        TransactionPool* txpool_;

        bool AppendGenesis(Block* block);

        inline BlockChainNode*
        GetBlockParent(Block* block){
            std::string target = block->GetPreviousHash();
            auto found = nodes_.find(target);
            if(found == nodes_.end()) return nullptr;
            return found->second;
        }

        BlockChain():
            heads_(new Array<BlockChainNode*>(0xA)),
            nodes_(),
            height_(0){}
    public:
        class Server;

        static BlockChain* GetInstance();
        static Server* GetServerInstance();

        Block* CreateBlock() const{
            return new Block(GetHead());
        }

        Block* GetBlockFromHash(const std::string& hash) const{
            return nodes_.find(hash)->second->GetBlock();
        }

        Block* GetHead() const{
            return head_->GetBlock();
        }

        UnclaimedTransactionPool* GetHeadUnclaimedTransactionPool(){
            return head_->GetUnclainedTransactionPool();
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

        TransactionPool* GetTransactionPool() const{
            return txpool_;
        }

        //TODO: Remove this
        void SetHead(Block* block){
            nodes_.clear();
            heads_->Clear();
            height_ = 0;
            AppendGenesis(block);
        }

        bool Append(Block* block);
        bool Load(const std::string& root);
        bool Load(const std::string& root, const std::string& addr, int port);
        bool Save(const std::string& root);

        class Server{
        private:
            static const size_t MAX_BUFF_SIZE = 4096;

            uv_loop_t loop_;
            uv_tcp_t server_;
            ByteBuffer read_buff_;
            bool running_;
            User user_;
            BlockChain* chain_;
            std::map<uv_stream_t*, Session*> sessions_;

            void OnNewConnection(uv_stream_t* server, int status);
            void OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff);
            void OnMessageSent(uv_write_t* stream, int status);
            void Send(uv_stream_t* conn, Message* msg);

            void RemoveClient(uv_stream_t* client){
                //TODO: Implement
            }

            bool Handle(Session* session, Message* msg);

            Server():
                    user_("TestUser"),
                    sessions_(),
                    running_(false),
                    server_(),
                    loop_(),
                    read_buff_(MAX_BUFF_SIZE){}
            friend class BlockChain;
        public:
            static void AddPeer(const std::string& addr, int port);
            static int Initialize(int port);
            static int Initialize(int port, const std::string& paddr, int pport);
            ~Server(){}

            void Broadcast(uv_stream_t* from, Message* msg);

            ByteBuffer* GetReadBuffer(){
                return &read_buff_;
            }

            bool IsRunning() const{
                return running_;
            }
        };
    };
}

#endif //TOKEN_BLOCKCHAIN_H