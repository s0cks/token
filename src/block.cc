#include "block.h"

namespace Token{
    static Byte* NewByteArray(size_t size){
        return reinterpret_cast<Byte*>(malloc(sizeof(Byte) * size));
    }

    static void EncodeString(ByteBuffer* bb, std::string val){
        uint32_t size = static_cast<uint32_t>(val.size());
        bb->PutInt(size);
        uint8_t* bytes = new uint8_t[size + 1];
        std::strcpy(reinterpret_cast<char*>(bytes), val.c_str());
        bb->PutBytes(bytes, size);
        delete[] bytes;
    }

    static void DecodeString(ByteBuffer* bb, std::string* result){
        uint32_t size = bb->GetInt();
        uint8_t* bytes = new uint8_t[size + 1];
        bb->GetBytes(bytes, size);
        bytes[size] = '\0';
        *result = std::string(reinterpret_cast<char*>(bytes));
        delete[] bytes;
    }

    void Input::Encode(Token::ByteBuffer* bb){
        bb->PutInt(GetIndex());
        EncodeString(bb, GetPreviousHash());
    }

    Input* Input::Decode(Token::ByteBuffer* bb){
        uint32_t idx = bb->GetInt();
        std::string prev_hash;
        DecodeString(bb, &prev_hash);
        return new Input(prev_hash, idx);
    }

    void Output::Encode(Token::ByteBuffer* bb){
        bb->PutInt(GetIndex());
        EncodeString(bb, GetUser());
        EncodeString(bb, GetToken());
    }

    Output* Output::Decode(Token::ByteBuffer* bb){
        uint32_t idx = bb->GetInt();
        std::string user;
        DecodeString(bb, &user);
        std::string token;
        DecodeString(bb, &token);
        return new Output(token, user, idx);
    }

    void Transaction::Accept(Token::TransactionVisitor* vis) const{
        int i;
        for(i = 0; i < GetNumberOfInputs(); i++){
            vis->VisitInput(GetInputAt(i));
        }
        for(i = 0; i < GetNumberOfOutputs(); i++){
            vis->VisitOutput(GetOutputAt(i));
        }
    }

    void Transaction::Encode(Token::ByteBuffer* bb) const{
        bb->PutInt(GetIndex());
        bb->PutInt(static_cast<uint32_t>(GetNumberOfInputs()));
        for(auto& it : inputs_){
            it->Encode(bb);
        }
        bb->PutInt(static_cast<uint32_t>(GetNumberOfOutputs()));
        for(auto& it : outputs_){
            it->Encode(bb);
        }
    }

    Transaction* Transaction::Decode(Token::ByteBuffer* bb){
        uint32_t idx = bb->GetInt();
        Transaction* tx = new Transaction(idx);

        uint32_t pos = 0;
        uint32_t len = bb->GetInt();
        for(pos = 0; pos < len; pos++){
            tx->AddInput(Input::Decode(bb));
        }

        len = bb->GetInt();
        for(pos = 0; pos < len; pos++){
            tx->AddOutput(Output::Decode(bb));
        }
        return tx;
    }

    class TransactionSizeCalculator : public TransactionVisitor{
    private:
        size_t size_;
    public:
       explicit TransactionSizeCalculator():
            size_(sizeof(uint32_t) * 3){}
        ~TransactionSizeCalculator(){}

        void VisitInput(Input* in){
            size_ += in->GetSize();
        }

        void VisitOutput(Output* out){
            size_ += out->GetSize();
        }

        size_t GetSize() const{
            return size_;
        }
    };

    std::string Transaction::GetHash() const{
        CryptoPP::SHA256 func;
        std::string digest;
        ByteBuffer bb;
        Encode(&bb);
        CryptoPP::ArraySource source(bb.GetBytes(), bb.Size(), true, new CryptoPP::HashFilter(func, new CryptoPP::HexEncoder(new CryptoPP::StringSink(digest))));
        return digest;
    }

    HashArray Transaction::GetHashArray() const{
        HashArray result;
        CryptoPP::SHA256 func;
        ByteBuffer bb;
        Encode(&bb);
        CryptoPP::ArraySource source(bb.GetBytes(), bb.Size(), true, new CryptoPP::HashFilter(func, new CryptoPP::ArraySink(result.data(), DIGEST_SIZE)));
        return result;
    }

    void Block::Encode(Token::ByteBuffer* bb) const{
        EncodeString(bb, GetPreviousHash());
        bb->PutInt(GetHeight());
        bb->PutInt(static_cast<uint32_t>(GetNumberOfTransactions()));
        for(size_t i = 0; i < GetNumberOfTransactions(); i++){
            GetTransactionAt(i)->Encode(bb);
        }
    }

    Block* Block::Decode(Token::ByteBuffer* bb){
        std::string prev_hash;
        DecodeString(bb, &prev_hash);
        int height = bb->GetInt();
        Block* block = new Block(prev_hash, static_cast<uint32_t>(height));
        int num_tx = bb->GetInt();
        for(int i = 0; i < num_tx; i++){
            block->AddTransaction(Transaction::Decode(bb));
        }
        return block;
    }

    Block* Block::Load(const std::string& filename){
        std::fstream fd(filename, std::ios::binary|std::ios::in);
        if(!fd){
            std::cerr << std::strerror(errno) << std::endl;
            return nullptr;
        }
        Token::ByteBuffer bb(fd);
        return Decode(&bb);
    }

    void Block::Write(const std::string& filename){
        std::fstream fd(filename, std::ios::binary|std::ios::out|std::ios::trunc);
        Token::ByteBuffer bb;
        Encode(&bb);
        fd.write(reinterpret_cast<char*>(bb.GetBytes()), bb.WrittenBytes());
        fd.flush();
    }

    std::string Block::GetHash() const{
        CryptoPP::SHA256 func;
        std::string digest;
        ByteBuffer bb;
        Encode(&bb);
        CryptoPP::ArraySource source(bb.GetBytes(), bb.Size(), true, new CryptoPP::HashFilter(func, new CryptoPP::HexEncoder(new CryptoPP::StringSink(digest))));
        return digest;
    }

    std::string Block::GetMerkleRoot() const{
        std::vector<MerkleNodeItem*> items;
        for(size_t i = 0; i < GetNumberOfTransactions(); i++) items.push_back(GetTransactionAt(i));
        MerkleTree mktree(items);
        return mktree.GetMerkleRoot();
    }
}