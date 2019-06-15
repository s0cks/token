#include "blockchain.h"
#include <fstream>
#include <sstream>
#include "block_validator.h"
#include "message.h"

namespace Token{
    bool
    BlockChain::AppendGenesis(Token::Block* genesis){
        Transaction* cb = genesis->GetCoinbaseTransaction();
        BlockChainNode* node = new BlockChainNode(nullptr, genesis);
        for(int i = 0; i < cb->GetNumberOfOutputs(); i++){
            std::cout << "Creating unclaimed transaction for " << i << std::endl;
            node->GetUnclainedTransactionPool()->Insert(UnclaimedTransaction(cb->GetHash(), i), cb->GetOutputAt(i));
        }
        heads_->Add(node);
        nodes_.insert(std::make_pair(genesis->GetHash(), node));
        height_ = 1;
        head_ = node;
        txpool_ = new TransactionPool();
        return true;
    }

    static inline bool
    FileExists(const std::string& name){
        std::ifstream f(name.c_str());
        return f.good();
    }

    BlockChain*
    BlockChain::GetInstance(){
        static BlockChain instance;
        return &instance;
    }

    BlockChain::Server*
    BlockChain::GetServerInstance(){
        static BlockChain::Server instance;
        return &instance;
    }

    bool
    BlockChain::Load(const std::string& root, const std::string& addr, int port){
        Load(root);
        std::cout << "Fetching head from " << addr << ":" << port << std::endl;
        return true;
    }

    bool
    BlockChain::Load(const std::string& root){
        Block* genesis = nullptr;

        std::string gen_filename = root + "/blk0.dat";
        if(FileExists(gen_filename)){
            genesis = Block::Load(gen_filename);
            AppendGenesis(genesis);
        } else{
            genesis = new Block();
            Transaction* tx = genesis->CreateTransaction();
            for(int idx = 0; idx < 128; idx++){
                std::stringstream token_name;
                token_name << "Token" << idx;
                tx->AddOutput("TestUser", token_name.str());
            }
            AppendGenesis(genesis);
            return false;
        }
        for(int idx = 1; idx < 1000; idx++){
            std::stringstream blk_filename;
            blk_filename << root << "/blk" << idx << ".dat";
            if(FileExists(blk_filename.str())){
                Append(Block::Load(blk_filename.str()));
            } else{
                break;
            }
        }
        return true;
    }

    static inline std::string
    GetBlockFilename(const std::string& root, int idx) {
        std::stringstream ss;
        ss << root;
        ss << "/blk" << idx << ".dat";
        return ss.str();
    }

    bool
    BlockChain::Save(const std::string& root){
        for(int idx = 0; idx <= GetHeight(); idx++){
            Block* blk = GetBlockAt(idx);
            blk->Write(GetBlockFilename(root, idx));
        }
        return true;
    }

    bool BlockChain::Append(Token::Block* block){
        if(nodes_.find(block->GetHash()) != nodes_.end()){
            std::cout << "Duplicate block found" << std::endl;
            return false;
        }
        if(block->GetHeight() == 0) return AppendGenesis(block);
        if(nodes_.find(block->GetPreviousHash()) == nodes_.end()){
            std::cout << "Cannot find parent" << std::endl;
            return false;
        }
        BlockChainNode* parent = GetBlockParent(block);
        UnclaimedTransactionPool utxo_pool = *parent->GetUnclainedTransactionPool();
        /*
        BlockValidator validator(utxo_pool);
        block->Accept(&validator);
        std::vector<Transaction*> valid = validator.GetValidTransactions();
        std::vector<Transaction*> invalid = validator.GetInvalidTransactions();
        if(valid.size() != block->GetNumberOfTransactions()){
            std::cerr << "Not all transactions valid" << std::endl;
            return false;
        }
        utxo_pool = validator.GetUnclaimedTransactionPool();
        if(utxo_pool.IsEmpty()) {
            std::cout << "No Unclaimed transactions" << std::endl;
            return false;
        }
        */

        Transaction* cb = block->GetCoinbaseTransaction();
        utxo_pool.Insert(UnclaimedTransaction(cb->GetHash(), 0), cb->GetOutputAt(0));
        BlockChainNode* current = new BlockChainNode(parent, block, utxo_pool);
        nodes_.insert(std::make_pair(block->GetHash(), current));
        if(current->GetHeight() > GetHeight()) {
            height_ = current->GetHeight();
            head_ = current;
        }
        if(GetHeight() - (*heads_)[0]->GetHeight() > 0xA){
            Array<BlockChainNode*>* newHeads = new Array<BlockChainNode*>(0xA);
            for(size_t i = 0; i < heads_->Length(); i++){
                BlockChainNode* oldHead = (*heads_)[i];
                Array<BlockChainNode*> oldHeadChildren = oldHead->GetChildren();
                for(size_t j = 0; j < oldHeadChildren.Length(); j++){
                    BlockChainNode* oldHeadChild = oldHeadChildren[j];
                    newHeads->Add(oldHeadChild);
                }
                nodes_.erase(oldHead->GetBlock()->GetHash());
            }
            heads_ = newHeads;
        }
        return true;
    }

    static void
    AllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buff){
        BlockChain::Server* instance = BlockChain::GetServerInstance();
        ByteBuffer* read = instance->GetReadBuffer();
        *buff = uv_buf_init((char*)read->GetBytes(), suggested_size);
    }

    void BlockChain::Server::AddPeer(const std::string &addr, int port){
        BlockChain::Server* instance = GetServerInstance();
        PeerSession* peer = new PeerSession(&instance->loop_, addr, port);
        instance->sessions_.insert({ peer->GetStream(), peer });
    }

    int BlockChain::Server::Initialize(int port, const std::string &paddr, int pport){
        BlockChain::Server* instance = GetServerInstance();
        instance->AddPeer(paddr, pport);
        return Initialize(port);
    }

    int BlockChain::Server::Initialize(int port){
        BlockChain::Server* instance = GetServerInstance();
        uv_loop_init(&instance->loop_);
        uv_tcp_init(&instance->loop_, &instance->server_);

        sockaddr_in addr;
        uv_ip4_addr("0.0.0.0", port, &addr);
        uv_tcp_bind(&instance->server_, (const struct sockaddr*)&addr, 0);
        int err = uv_listen((uv_stream_t*)&instance->server_,
                            100,
                            [](uv_stream_t* server, int status){
                                BlockChain::GetServerInstance()->OnNewConnection(server, status);
                            });
        if(err == 0){
            std::cout << "Listening @ localhost:" << port << std::endl;
            uv_run(&instance->loop_, UV_RUN_DEFAULT);
            instance->running_ = true;
        }
        return err;
    }

    void BlockChain::Server::OnNewConnection(uv_stream_t* server, int status){
        std::cout << "Attempting connection..." << std::endl;
        if(status == 0){
            ClientSession* session = new ClientSession(std::make_shared<uv_tcp_t>());
            uv_tcp_init(&loop_, session->GetConnection());
            if(uv_accept(server, (uv_stream_t*)session->GetConnection()) == 0){
                std::cout << "Accepted!" << std::endl;
                uv_stream_t* key = (uv_stream_t*)session->GetConnection();
                uv_read_start((uv_stream_t*) session->GetConnection(),
                              AllocBuffer,
                              [](uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff){
                                  BlockChain::GetServerInstance()->OnMessageReceived(stream, nread, buff);
                              });
                sessions_.insert({ key, session });
            } else{
                std::cerr << "Error accepting connection: " << std::string(uv_strerror(status));
                uv_read_stop((uv_stream_t*) session->GetConnection());
            }
        } else{
            std::cerr << "Connection error: " << std::string(uv_strerror(status));
        }
    }

    void BlockChain::Server::Broadcast(uv_stream_t* stream, Message* msg){
        for(auto& it : sessions_){
            if(it.second->IsPeerSession() && it.first != stream){
                it.second->Send(msg);
            }
        }
    }

    void BlockChain::Server::Send(uv_stream_t* target, Message* msg){
        ByteBuffer bb;
        msg->Encode(&bb);

        uv_buf_t buff = uv_buf_init((char*) bb.GetBytes(), bb.WrittenBytes());
        uv_write_t* req = (uv_write_t*) malloc(sizeof(uv_write_t));
        req->data = buff.base;
        uv_write(req, target, &buff, 1, [](uv_write_t* req, int status){
            BlockChain::GetServerInstance()->OnMessageSent(req, status);
        });
    }

    void BlockChain::Server::OnMessageSent(uv_write_t* req, int status){
        if(status != 0){
            std::cerr << "Failed to send message: " << std::string(uv_strerror(status)) << std::endl;
            if(req) free(req);
        }
    }

    bool BlockChain::Server::Handle(Session* session, Token::Message* msg){
        if(msg->IsRequest()){
            Request* request = msg->AsRequest();
            if(request->IsGetHeadRequest()){
                Block* head = BlockChain::GetInstance()->GetHead();
                GetHeadResponse response((*request->AsGetHeadRequest()), head);
                session->Send(&response);
                return true;
            }
        }
        return false;
    }

    void BlockChain::Server::OnMessageReceived(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buff){
        if(nread == UV_EOF){
            RemoveClient(stream);
            std::cout << "Disconnected" << std::endl;
        }

        auto pos = sessions_.find(stream);
        if(pos != sessions_.end()){
            if(nread == UV_EOF){
                RemoveClient(stream);
                std::cout << "Disconnected" << std::endl;
            } else if(nread > 0){
                Session* session = pos->second;
                if(nread < 4096){
                    session->Append(buff);
                    if(!Handle(session, session->GetNextMessage())){
                        std::cerr << "BlockChain::Server couldn't handle message from: " << session << std::endl;
                    }
                }
            } else if(nread < 0){
                std::cerr << "Error: " << std::to_string(nread) << std::endl;
            } else if(nread == 0){
                std::cerr << "Zero message size received" << std::endl;
            }
        } else{
            uv_read_stop(stream);
            std::cerr << "Unrecognized client. Disconnecting" << std::endl;
        }
    }

    class Session{

    };
}