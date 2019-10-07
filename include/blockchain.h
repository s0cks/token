#ifndef TOKEN_BLOCKCHAIN_H
#define TOKEN_BLOCKCHAIN_H

#include <leveldb/db.h>
#include <map>

#include "common.h"
#include "block.h"
#include "user.h"
#include "utxo.h"
#include "node/peer.h"

namespace Token{
    class BlockChainVisitor{
    public:
        BlockChainVisitor(){}
        virtual ~BlockChainVisitor() = default;
        virtual bool Visit(Block* block) = 0;
    };

    class BlockChain{
    public:
        static const size_t kKeypairSize = 1024;
        static const uintptr_t kBlockChainInitSize = 1024;
    private:
        friend class BlockChainServer;
        friend class PeerSession;
        friend class PeerClient;

        std::string path_;
        leveldb::DB* state_;
        pthread_rwlock_t rwlock_;
        CryptoPP::RSA::PublicKey pubkey_;
        CryptoPP::RSA::PrivateKey privkey_;

        Block** blocks_;
        uintptr_t blocks_caps_;
        uintptr_t blocks_size_;

        static inline leveldb::DB*
        GetState(){
            return GetInstance()->state_;
        }

        bool CreateGenesis(); //TODO: Remove

        bool LoadBlockChain(const std::string& path);
        bool GenerateChainKeys(const std::string& pubkey, const std::string& privkey);
        bool InitializeChainKeys(const std::string& path);
        bool InitializeChainState(const std::string& root);

        bool GetReference(const std::string& ref, uint32_t* height); //TODO: Remove
        bool SetReference(const std::string& ref, uint32_t height); //TODO: Remove

        bool AppendNode(Block* block);
        bool SaveBlock(Block* block);
        bool LoadBlock(uint32_t height, Block** result);
        bool DeleteBlock(uint32_t height);

        void Resize(uintptr_t nlen){
            if(nlen > blocks_caps_){
                uintptr_t ncaps = RoundUpPowTwo(nlen);
                Block** ndata = (Block**)realloc(blocks_, ncaps);
                blocks_ = ndata;
                blocks_caps_ = ncaps;
            }
            blocks_size_ = nlen;
        }

        uintptr_t Length() const{
            return blocks_size_;
        }

        uintptr_t Capacity() const{
            return blocks_caps_;
        }

        Block*& operator[](size_t idx) const{
            return blocks_[idx];
        }

        Block*& Last() const{
            return operator[](Length() - 1);
        }

        BlockChain():
            rwlock_(),
            path_(),
            blocks_(nullptr),
            blocks_size_(0),
            blocks_caps_(RoundUpPowTwo(kBlockChainInitSize)),
            state_(nullptr){
            blocks_ = (Block**)(malloc(sizeof(Block*) * blocks_caps_));
            memset(blocks_, 0, sizeof(Block*) * blocks_caps_);
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
        static bool GetBlockList(std::vector<std::string>& blocks);
        static bool HasHead();
        static bool ContainsBlock(const std::string& hash);
        static bool AppendBlock(Block* block);
        static bool Initialize(const std::string& path);
        static bool Clear();
    };

    class BlockChainPrinter : public BlockChainVisitor{
    private:
        bool info_;

        bool ShouldPrintInfo(){
            return info_;
        }

        BlockChainPrinter(bool info):
                info_(info){}
    public:
        ~BlockChainPrinter(){}

        bool Visit(Block* block);

        static void PrintBlockChain(bool info=false);
    };
}

#endif //TOKEN_BLOCKCHAIN_H