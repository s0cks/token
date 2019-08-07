#include "utxo.h"
#include <cstring>

namespace Token{
    UnclaimedTransactionPool::UnclaimedTransactionPool():
        db_path_(),
        database_(nullptr){}

    UnclaimedTransactionPool::~UnclaimedTransactionPool(){
        sqlite3_close(database_);
    }

    UnclaimedTransactionPool* UnclaimedTransactionPool::GetInstance(){
        static UnclaimedTransactionPool instance;
        return &instance;
    }

    bool UnclaimedTransactionPool::GetUnclaimedTransactions(std::vector<UnclaimedTransaction*>& utxos){
        utxos.clear();
        std::string sql = "SELECT UTxHash,TxHash,TxIndex,User,Token FROM UnclaimedTransactions;";
        char* err_msg = NULL;

        sqlite3_stmt* stmt;
        int rc;
        if((rc = sqlite3_prepare_v2(database_, sql.c_str(), -1, &stmt, NULL)) != SQLITE_OK){
            std::cerr << "Couldn't prepare statement: " << err_msg << std::endl;
            return false;
        }
        while((rc = sqlite3_step(stmt)) == SQLITE_ROW){
            std::string utx_hash = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
            std::string tx_hash = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
            int index = sqlite3_column_int(stmt, 2);
            std::string user = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
            std::string token = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)));
            utxos.push_back(new UnclaimedTransaction(tx_hash, index, user, token));
        }
        sqlite3_finalize(stmt);
        return utxos.size() > 0;
    }

    bool UnclaimedTransactionPool::GetUnclaimedTransaction(const std::string& tx_hash, int index, UnclaimedTransaction* result){
        std::string sql = "SELECT UTxHash,TxHash,TxIndex,User,Token FROM UnclaimedTransactions WHERE TxHash=@TxHash AND TxIndex=@TxIndex;";
        char* err_msg = NULL;

        sqlite3_stmt* stmt;
        int rc;
        if((rc = sqlite3_prepare_v2(database_, sql.c_str(), -1, &stmt, NULL)) != SQLITE_OK){
            std::cerr << "Couldn't prepare statement: " << err_msg << std::endl;
            return false;
        }
        int param_idx = sqlite3_bind_parameter_index(stmt, "@TxHash");
        if((rc = sqlite3_bind_text(stmt, param_idx, tx_hash.c_str(), -1, SQLITE_TRANSIENT)) != SQLITE_OK){
            std::cerr << "Couldn't bind TxHash" << std::endl;
            return false;
        }

        param_idx = sqlite3_bind_parameter_index(stmt, "@TxIndex");
        if((rc = sqlite3_bind_int(stmt, param_idx, index)) != SQLITE_OK){
            std::cerr << "Couldn't bind TxIndex" << std::endl;
            return false;
        }
        while((rc = sqlite3_step(stmt)) == SQLITE_ROW){
            std::string utx_hash = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
            std::string tx_hash = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
            int index = sqlite3_column_int(stmt, 2);
            std::string user = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
            std::string token = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)));
            *result = UnclaimedTransaction(tx_hash, index, user, token);
            return true;
        }
        sqlite3_finalize(stmt);
        return false;
    }

    bool UnclaimedTransactionPool::RemoveUnclaimedTransaction(Token::UnclaimedTransaction* utxo){
        std::string sql = "DELETE FROM UnclaimedTransactions WHERE TxHash=@TxHash AND TxIndex=@TxIndex";

        sqlite3_stmt* stmt;
        int rc;
        if((rc = sqlite3_prepare_v2(database_, sql.c_str(), -1, &stmt, NULL)) != SQLITE_OK){
            std::cerr << "Couldn't prepare statement: " << sqlite3_errmsg(database_) << std::endl;
            return false;
        }

        int param_idx = sqlite3_bind_parameter_index(stmt, "@TxHash");
        if((rc = sqlite3_bind_text(stmt, param_idx, utxo->GetTransactionHash().c_str(), -1, SQLITE_TRANSIENT)) != SQLITE_OK){
            std::cerr << "Couldn't bind TxHash" << std::endl;
            return false;
        }

        param_idx = sqlite3_bind_parameter_index(stmt, "@TxIndex");
        if((rc = sqlite3_bind_int(stmt, param_idx, utxo->GetIndex())) != SQLITE_OK){
            std::cerr << "Couldn't bind TxIndex" << std::endl;
            return false;
        }

        if((rc = sqlite3_step(stmt)) != SQLITE_DONE){
            std::cerr << "Unable to remove value:" << sqlite3_errmsg(database_) << std::endl;
            sqlite3_free(stmt);
            return false;
        }
        sqlite3_finalize(stmt);
        return true;
    }

    bool UnclaimedTransactionPool::AddUnclaimedTransaction(Token::UnclaimedTransaction* utxo){
        std::string sql = "INSERT OR IGNORE INTO UnclaimedTransactions (UTxHash, TxHash, TxIndex, User, Token) VALUES (@UTxHash, @TxHash, @TxIndex, @User, @Token);";

        sqlite3_stmt* stmt;
        int rc;
        if((rc = sqlite3_prepare_v2(database_, sql.c_str(), -1, &stmt, NULL)) != SQLITE_OK) {
            std::cerr << "Couldn't prepare statement: " << sqlite3_errmsg(database_) << std::endl;
            return false;
        }

        int param_idx = sqlite3_bind_parameter_index(stmt, "@UTxHash");
        if((rc = sqlite3_bind_text(stmt, param_idx, utxo->GetHash().c_str(), -1, SQLITE_TRANSIENT)) != SQLITE_OK){
            std::cerr << "Couldn't bind UTxHash" << std::endl;
            return false;
        }

        param_idx = sqlite3_bind_parameter_index(stmt, "@TxHash");
        if((rc = sqlite3_bind_text(stmt, param_idx, utxo->GetTransactionHash().c_str(), -1, SQLITE_TRANSIENT)) != SQLITE_OK){
            std::cerr << "Couldn't bind TxHash" << std::endl;
            return false;
        }

        param_idx = sqlite3_bind_parameter_index(stmt, "@TxIndex");
        if((rc = sqlite3_bind_int(stmt, param_idx, utxo->GetIndex())) != SQLITE_OK){
            std::cerr << "Couldn't bind TxIndex" << std::endl;
            return false;
        }

        param_idx = sqlite3_bind_parameter_index(stmt, "@User");
        if((rc = sqlite3_bind_text(stmt, param_idx, utxo->GetUser().c_str(), -1, SQLITE_TRANSIENT)) != SQLITE_OK){
            std::cerr << "Couldn't bind User" << std::endl;
            return false;
        }

        param_idx = sqlite3_bind_parameter_index(stmt, "@Token");
        if((rc = sqlite3_bind_text(stmt, param_idx, utxo->GetToken().c_str(), -1, SQLITE_TRANSIENT)) != SQLITE_OK){
            std::cerr << "Couldn't bind Token" << std::endl;
            return false;
        }

        if((rc = sqlite3_step(stmt)) != SQLITE_DONE){
            std::cerr << "Unable to insert value:" << sqlite3_errmsg(database_) << std::endl;
            sqlite3_free(stmt);
            return false;
        }
        sqlite3_finalize(stmt);
        return true;
    }

    bool UnclaimedTransactionPool::LoadUnclaimedTransactionPool(const std::string &filename) {
        UnclaimedTransactionPool *instance = GetInstance();
        int rc;
        if ((rc = sqlite3_open(filename.c_str(), &instance->database_)) != SQLITE_OK) {
            std::cerr << "Unable to open UnclaimedTransactionPool @" << filename << std::endl;
            sqlite3_close(instance->database_);
            return false;
        }
        std::string sql = "CREATE TABLE IF NOT EXISTS UnclaimedTransactions ("
                          "UTxHash CHAR(65) PRIMARY KEY NOT NULL,"
                          "TxHash CHAR(65) NOT NULL,"
                          "TxIndex INT NOT NULL,"
                          "User CHAR(65) NOT NULL,"
                          "Token CHAR(65) NOT NULL"
                          ");";
        char *err_msg = 0;
        if ((rc = sqlite3_exec(instance->database_, sql.c_str(), NULL, NULL, &err_msg)) != SQLITE_OK) {
            std::cerr << "Couldn't create accounts table:" << err_msg << std::endl;
            sqlite3_free(err_msg);
            sqlite3_close(instance->database_);
            return false;
        }
        return true;
    }

    void UnclaimedTransaction::Encode(Token::ByteBuffer *bb) const{
        uint32_t size = static_cast<uint32_t>(GetRaw()->ByteSizeLong());
        bb->Resize(size);
        GetRaw()->SerializeToArray(bb->GetBytes(), size);
    }

    std::string UnclaimedTransaction::GetHash() const{
        CryptoPP::SHA256 func;
        std::string digest;
        ByteBuffer bb;
        Encode(&bb);
        CryptoPP::ArraySource source(bb.GetBytes(), bb.Size(), true, new CryptoPP::HashFilter(func, new CryptoPP::HexEncoder(new CryptoPP::StringSink(digest))));
        return digest;
    }
}