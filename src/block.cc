#include "block.h"

namespace Token{
    static Byte* NewByteArray(size_t size){
        return reinterpret_cast<Byte*>(malloc(sizeof(Byte) * size));
    }

    void Transaction::Accept(Token::TransactionVisitor* vis){
        int i;
        for(i = 0; i < GetNumberOfInputs(); i++){
            vis->VisitInput(GetInputAt(i));
        }
        for(i = 0; i < GetNumberOfOutputs(); i++){
            vis->VisitOutput(GetOutputAt(i));
        }
    }

    Byte* Transaction::GetData() const{
        size_t size = GetDataSize();
        Byte* bytes = NewByteArray(size);
        GetRaw()->SerializeToArray(bytes, size);
        return bytes;
    }

    std::string Transaction::GetHash(){
        Byte* bytes = GetData();
        size_t size = GetDataSize();
        CryptoPP::SHA256 hash;
        std::string digest;
        CryptoPP::ArraySource source(bytes, size, true, new CryptoPP::HashFilter(hash, new CryptoPP::HexEncoder(new CryptoPP::StringSink(digest))));
        free(bytes);
        return digest;
    }

    Transaction* Block::CreateTransaction(){
        int index = GetNumberOfTransactions();
        Messages::Transaction* raw_tx = GetRaw()->add_transactions();
        Transaction* tx = new Transaction(raw_tx, index);
        if(tx->IsOrigin()) GetRaw()->set_allocated_origin(raw_tx);
        return tx;
    }

    void Block::Accept(Token::BlockVisitor* vis){
        int i;
        for(i = 0; i < GetNumberOfTransactions(); i++){
            vis->VisitTransaction(GetTransactionAt(i));
        }
    }

    void Block::Write(const std::string &filename){
        std::fstream output(filename, std::ios::out|std::ios::trunc|std::ios::binary);
        if(!GetRaw()->SerializeToOstream(&output)){
            std::cerr << "Failed to write to file " << filename << std::endl;
        }
    }

    class BlockMerkleRootCalculator : public BlockVisitor{
    private:
        std::vector<MerkleNodeItem*> transactions_;
    public:
        BlockMerkleRootCalculator():
            transactions_(){
        }
        ~BlockMerkleRootCalculator(){
            if(!transactions_.empty()){
                int i;
                for(i = 0; i < transactions_.size(); i++){
                    delete transactions_[i];
                }
            }
        }

        void VisitTransaction(Transaction* tx){
            transactions_.push_back(tx);
        }

        std::string GetMerkleRoot(){
            MerkleTree mk_tree(transactions_);
            return mk_tree.GetMerkleRoot();
        }
    };

    std::string Block::ComputeMerkleRoot(){
        BlockMerkleRootCalculator calc;
        Accept(&calc);
        return calc.GetMerkleRoot();
    }

    std::string Block::ComputeHash(){
        size_t size = GetRaw()->ByteSizeLong();
        Byte* bytes = NewByteArray(size);
        GetRaw()->SerializeToArray(bytes, size);

        std::string digest;
        HashFunction func;
        CryptoPP::ArraySource source(bytes, size, true, new CryptoPP::HashFilter(func, new CryptoPP::HexEncoder(new CryptoPP::StringSink(digest))));
        return digest;
    }

    std::string Block::GetHash(){
        if(GetRaw()->merkle_root().empty()) GetRaw()->set_merkle_root(ComputeMerkleRoot());
        if(GetRaw()->hash().empty()) GetRaw()->set_hash(ComputeHash());
        return GetRaw()->hash();
    }

    std::string Block::GetMerkleRoot(){
        if(GetRaw()->merkle_root().empty()) GetRaw()->set_merkle_root(ComputeMerkleRoot());
        return GetRaw()->merkle_root();
    }

    std::ostream& operator<<(std::ostream& stream, const Block& block){
        block.GetRaw()->SerializePartialToOstream(&stream);
        return stream;
    }
}