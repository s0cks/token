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
    class ChainNode{
    private:
        ChainNode* prev_;
        ChainNode* next_;
        Block* block_;

        void SetPrevious(ChainNode* node){
            prev_ = node;
        }

        void SetNext(ChainNode* node){
            next_ = node;
        }

        ChainNode(Block* block):
            prev_(nullptr),
            next_(nullptr),
            block_(block){}

        friend class BlockChain;
    public:
        ~ChainNode(){}

        ChainNode* GetPrevious() const{
            return prev_;
        }

        ChainNode* GetNext() const{
            return next_;
        }

        Block* GetBlock() const{
            return block_;
        }

        std::string GetHash() const{
            return GetBlock()->GetHash();
        }

        uint32_t GetHeight() const {
            return GetBlock()->GetHeight();
        }
    };

    class BlockChainVisitor{
    public:
        BlockChainVisitor(){}
        virtual ~BlockChainVisitor() = default;
        virtual bool Visit(Block* block) = 0;
    };

    class BlockChain{
    public:
        static const size_t kKeypairSize = 4096;
    private:
        friend class BlockChainServer;
        friend class PeerSession;
        friend class PeerClient;

        std::string path_;
        leveldb::DB* state_;
        pthread_rwlock_t rwlock_;
        CryptoPP::RSA::PublicKey pubkey_;
        CryptoPP::RSA::PrivateKey privkey_;
        ChainNode* genesis_;
        ChainNode* head_;

        static inline leveldb::DB*
        GetState(){
            return GetInstance()->state_;
        }

        bool CreateGenesis(); //TODO: Remove
        bool LoadBlockChain(const std::string& path); //TODO: Remove
        bool GenerateChainKeys(const std::string& pubkey, const std::string& privkey);
        bool InitializeChainKeys(const std::string& path);
        bool InitializeChainState(const std::string& root);
        bool GetReference(const std::string& ref, uint32_t* height);
        bool SetReference(const std::string& ref, uint32_t height);
        bool AppendNode(Block* block);
        bool SaveBlock(Block* block);
        bool LoadBlock(const std::string& hash, Block** result);
        bool LoadBlock(uint32_t height, Block** result);
        std::string GetBlockFile(uint32_t height);
        std::string GetBlockFile(const std::string& hash);
        ChainNode* GetNode(uint32_t height);

        void SetGenesisNode(ChainNode* node){
            genesis_ = node;
        }

        void SetHeadNode(ChainNode* node){
            head_ = node;
        }

        ChainNode* GetGenesisNode() const{
            return genesis_;
        }

        ChainNode* GetHeadNode() const{
            return head_;
        }

        BlockChain():
            rwlock_(),
            path_(),
            genesis_(nullptr),
            head_(nullptr),
            state_(nullptr){
            pthread_rwlock_init(&rwlock_, NULL);
        }
    public:
        static BlockChain* GetInstance();

        static CryptoPP::PublicKey* GetPublicKey(){
            return &GetInstance()->pubkey_;
        }

        static CryptoPP::PrivateKey* GetPrivateKey(){
            return &GetInstance()->privkey_;
        }

        static std::string GetRootDirectory();
        static void Accept(BlockChainVisitor* vis);
        static Block* GetHead();
        static Block* GetGenesis();
        static Block* GetBlock(const std::string& hash);
        static Block* GetBlock(uint32_t height);
        static uint32_t GetHeight();
        static bool HasHead();
        static bool ContainsBlock(const std::string& hash);
        static bool AppendBlock(Block* block);
        static bool Initialize(const std::string& path);
    };

    class BlockChainPrinter : public BlockChainVisitor{
    public:
        BlockChainPrinter(){}
        ~BlockChainPrinter(){}

        bool Visit(Block* block);

        static void PrintBlockChain();
    };
}

#endif //TOKEN_BLOCKCHAIN_H