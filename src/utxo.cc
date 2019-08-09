#include "utxo.h"
#include <cstring>

namespace Token{
    UnclaimedTransactionPool::UnclaimedTransactionPool():
        db_path_(),
        rwlock_(),
        database_(nullptr){
        pthread_rwlock_init(&rwlock_, NULL);
    }

    UnclaimedTransactionPool::~UnclaimedTransactionPool(){
        sqlite3_close(database_);
        pthread_rwlock_destroy(&rwlock_);
    }

    UnclaimedTransactionPool* UnclaimedTransactionPool::GetInstance(){
        static UnclaimedTransactionPool instance;
        return &instance;
    }

    bool UnclaimedTransactionPool::GetUnclaimedTransactions(const std::string& user, Messages::UnclaimedTransactionsList* utxos){
        pthread_rwlock_rdlock(&rwlock_);
        std::string sql = "SELECT UTxHash,TxHash,TxIndex,User,Token FROM UnclaimedTransactions WHERE User=@User;";
        char* err_msg = NULL;

        std::cout << "Getting unclaimed transactions for " << user << std::endl;

        sqlite3_stmt* stmt;
        int rc;
        if((rc = sqlite3_prepare_v2(database_, sql.c_str(), -1, &stmt, NULL)) != SQLITE_OK){
            std::cout << "Couldn't prepare statement: " << err_msg << std::endl;
            pthread_rwlock_unlock(&rwlock_);
            return false;
        }

        int param_idx = sqlite3_bind_parameter_index(stmt, "@User");
        if((rc = sqlite3_bind_text(stmt, param_idx, user.c_str(), -1, SQLITE_TRANSIENT)) != SQLITE_OK){
            std::cout << "Couldn't bind User" << std::endl;
            pthread_rwlock_unlock(&rwlock_);
            return false;
        }

        while((rc = sqlite3_step(stmt)) == SQLITE_ROW){
            Messages::UnclaimedTransaction* utxo = utxos->add_transactions();
            utxo->set_utx_hash(std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0))));
            utxo->set_tx_hash(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
            utxo->set_index(sqlite3_column_int(stmt, 2));
            utxo->set_user(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
            utxo->set_token(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)));
        }
        sqlite3_finalize(stmt);
        pthread_rwlock_unlock(&rwlock_);
        return true;
    }

    bool UnclaimedTransactionPool::GetUnclaimedTransactions(const std::string& user, std::vector<UnclaimedTransaction*>& utxos){
        pthread_rwlock_rdlock(&rwlock_);
        utxos.clear();
        std::string sql = "SELECT UTxHash,TxHash,TxIndex,User,Token FROM UnclaimedTransactions WHERE User=@User;";
        char* err_msg = NULL;

        sqlite3_stmt* stmt;
        int rc;
        if((rc = sqlite3_prepare_v2(database_, sql.c_str(), -1, &stmt, NULL)) != SQLITE_OK){
            std::cout << "Couldn't prepare statement: " << err_msg << std::endl;
            pthread_rwlock_unlock(&rwlock_);
            return false;
        }

        int param_idx = sqlite3_bind_parameter_index(stmt, "@User");
        if((rc = sqlite3_bind_text(stmt, param_idx, user.c_str(), -1, SQLITE_TRANSIENT)) != SQLITE_OK){
            std::cout << "Couldn't bind User" << std::endl;
            pthread_rwlock_unlock(&rwlock_);
            return false;
        }

        while((rc = sqlite3_step(stmt)) == SQLITE_ROW){
            std::string utx_hash = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
            std::cout << "Retrieved: " << utx_hash << std::endl;
            std::string tx_hash = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
            int index = sqlite3_column_int(stmt, 2);
            std::string u = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
            std::string token = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)));
            utxos.push_back(new UnclaimedTransaction(tx_hash, index, u, token));
        }
        sqlite3_finalize(stmt);
        pthread_rwlock_unlock(&rwlock_);
        return utxos.size() > 0;
    }

    bool UnclaimedTransactionPool::GetUnclaimedTransactions(std::vector<UnclaimedTransaction*>& utxos){
        pthread_rwlock_rdlock(&rwlock_);
        utxos.clear();
        std::string sql = "SELECT UTxHash,TxHash,TxIndex,User,Token FROM UnclaimedTransactions;";
        char* err_msg = NULL;

        sqlite3_stmt* stmt;
        int rc;
        if((rc = sqlite3_prepare_v2(database_, sql.c_str(), -1, &stmt, NULL)) != SQLITE_OK){
            std::cout << "Couldn't prepare statement: " << err_msg << std::endl;
            pthread_rwlock_unlock(&rwlock_);
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
        pthread_rwlock_unlock(&rwlock_);
        return utxos.size() > 0;
    }

    bool UnclaimedTransactionPool::GetUnclaimedTransactions(Messages::UnclaimedTransactionsList* utxos){
        pthread_rwlock_rdlock(&rwlock_);
        std::string sql = "SELECT UTxHash,TxHash,TxIndex,User,Token FROM UnclaimedTransactions;";
        char* err_msg = NULL;

        sqlite3_stmt* stmt;
        int rc;
        if((rc = sqlite3_prepare_v2(database_, sql.c_str(), -1, &stmt, NULL)) != SQLITE_OK){
            std::cout << "Couldn't prepare statement: " << err_msg << std::endl;
            pthread_rwlock_unlock(&rwlock_);
            return false;
        }
        while((rc = sqlite3_step(stmt)) == SQLITE_ROW){
            Messages::UnclaimedTransaction* utxo = utxos->add_transactions();
            utxo->set_utx_hash(std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0))));
            utxo->set_tx_hash(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
            utxo->set_index(sqlite3_column_int(stmt, 2));
            utxo->set_user(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
            utxo->set_token(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)));
        }
        sqlite3_finalize(stmt);
        pthread_rwlock_unlock(&rwlock_);
        return true;
    }

    bool UnclaimedTransactionPool::GetUnclaimedTransaction(const std::string& hash, UnclaimedTransaction** result){
        pthread_rwlock_rdlock(&rwlock_);
        std::string sql = "SELECT UTxHash,TxHash,TxIndex,User,Token FROM UnclaimedTransactions WHERE UTxHash=@UTxHash;";
        char* err_msg = NULL;

        sqlite3_stmt* stmt;
        int rc;
        if((rc = sqlite3_prepare_v2(database_, sql.c_str(), -1, &stmt, NULL)) != SQLITE_OK){
            std::cout << "Couldn't prepare statement: " << err_msg << std::endl;
            pthread_rwlock_unlock(&rwlock_);
            return false;
        }
        int param_idx = sqlite3_bind_parameter_index(stmt, "@UTxHash");
        if((rc = sqlite3_bind_text(stmt, param_idx, hash.c_str(), -1, SQLITE_TRANSIENT)) != SQLITE_OK){
            std::cout << "Couldn't bind TxIndex" << std::endl;
            pthread_rwlock_unlock(&rwlock_);
            return false;
        }
        while((rc = sqlite3_step(stmt)) == SQLITE_ROW){
            std::string utx_hash = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
            std::string tx_hash = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
            int index = sqlite3_column_int(stmt, 2);
            std::string user = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
            std::string token = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)));
            *result = new UnclaimedTransaction(tx_hash, index, user, token);
            pthread_rwlock_unlock(&rwlock_);
            return true;
        }
        sqlite3_finalize(stmt);
        pthread_rwlock_unlock(&rwlock_);
        return false;
    }

    bool UnclaimedTransactionPool::GetUnclaimedTransaction(const std::string& tx_hash, int index, UnclaimedTransaction** result){
        pthread_rwlock_rdlock(&rwlock_);
        std::string sql = "SELECT UTxHash,TxHash,TxIndex,User,Token FROM UnclaimedTransactions WHERE TxHash=@TxHash AND TxIndex=@TxIndex;";
        char* err_msg = NULL;

        sqlite3_stmt* stmt;
        int rc;
        if((rc = sqlite3_prepare_v2(database_, sql.c_str(), -1, &stmt, NULL)) != SQLITE_OK){
            std::cout << "Couldn't prepare statement: " << err_msg << std::endl;
            pthread_rwlock_unlock(&rwlock_);
            return false;
        }
        int param_idx = sqlite3_bind_parameter_index(stmt, "@TxHash");
        if((rc = sqlite3_bind_text(stmt, param_idx, tx_hash.c_str(), -1, SQLITE_TRANSIENT)) != SQLITE_OK){
            std::cout << "Couldn't bind TxHash" << std::endl;
            pthread_rwlock_unlock(&rwlock_);
            return false;
        }

        param_idx = sqlite3_bind_parameter_index(stmt, "@TxIndex");
        if((rc = sqlite3_bind_int(stmt, param_idx, index)) != SQLITE_OK){
            std::cout << "Couldn't bind TxIndex" << std::endl;
            pthread_rwlock_unlock(&rwlock_);
            return false;
        }
        while((rc = sqlite3_step(stmt)) == SQLITE_ROW){
            std::string utx_hash = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
            std::string tx_hash = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
            int index = sqlite3_column_int(stmt, 2);
            std::string user = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
            std::string token = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)));
            *result = new UnclaimedTransaction(tx_hash, index, user, token);
            pthread_rwlock_unlock(&rwlock_);
            return true;
        }
        sqlite3_finalize(stmt);
        pthread_rwlock_unlock(&rwlock_);
        return false;
    }

    bool UnclaimedTransactionPool::RemoveUnclaimedTransaction(Token::UnclaimedTransaction* utxo){
        pthread_rwlock_wrlock(&rwlock_);
        std::string sql = "DELETE FROM UnclaimedTransactions WHERE TxHash=@TxHash AND TxIndex=@TxIndex;";

        sqlite3_stmt* stmt;
        int rc;
        if((rc = sqlite3_prepare_v2(database_, sql.c_str(), -1, &stmt, NULL)) != SQLITE_OK){
            std::cout << "Couldn't prepare statement: " << sqlite3_errmsg(database_) << std::endl;
            sqlite3_free(stmt);
            pthread_rwlock_unlock(&rwlock_);
            return false;
        }

        int param_idx = sqlite3_bind_parameter_index(stmt, "@TxHash");
        if((rc = sqlite3_bind_text(stmt, param_idx, utxo->GetTransactionHash().c_str(), -1, SQLITE_TRANSIENT)) != SQLITE_OK){
            std::cout << "Couldn't bind TxHash" << std::endl;
            sqlite3_free(stmt);
            pthread_rwlock_unlock(&rwlock_);
            return false;
        }

        param_idx = sqlite3_bind_parameter_index(stmt, "@TxIndex");
        if((rc = sqlite3_bind_int(stmt, param_idx, utxo->GetIndex())) != SQLITE_OK){
            std::cout << "Couldn't bind TxIndex" << std::endl;
            sqlite3_free(stmt);
            pthread_rwlock_unlock(&rwlock_);
            return false;
        }

        if((rc = sqlite3_step(stmt)) != SQLITE_DONE){
            std::cout << "Unable to remove value:" << sqlite3_errmsg(database_) << std::endl;
            sqlite3_free(stmt);
            pthread_rwlock_unlock(&rwlock_);
            return false;
        }
        sqlite3_finalize(stmt);
        sqlite3_free(stmt);
        pthread_rwlock_unlock(&rwlock_);
        return true;
    }

    bool UnclaimedTransactionPool::AddUnclaimedTransaction(Token::UnclaimedTransaction* utxo){
        pthread_rwlock_wrlock(&rwlock_);
        std::string sql = "INSERT OR IGNORE INTO UnclaimedTransactions (UTxHash, TxHash, TxIndex, User, Token) VALUES (@UTxHash, @TxHash, @TxIndex, @User, @Token);";

        sqlite3_stmt* stmt;
        int rc;
        if((rc = sqlite3_prepare_v2(database_, sql.c_str(), -1, &stmt, NULL)) != SQLITE_OK) {
            std::cerr << "Couldn't prepare statement: " << sqlite3_errmsg(database_) << std::endl;
            pthread_rwlock_unlock(&rwlock_);
            return false;
        }

        int param_idx = sqlite3_bind_parameter_index(stmt, "@UTxHash");
        if((rc = sqlite3_bind_text(stmt, param_idx, utxo->GetHash().c_str(), -1, SQLITE_TRANSIENT)) != SQLITE_OK){
            std::cerr << "Couldn't bind UTxHash" << std::endl;
            pthread_rwlock_unlock(&rwlock_);
            return false;
        }

        param_idx = sqlite3_bind_parameter_index(stmt, "@TxHash");
        if((rc = sqlite3_bind_text(stmt, param_idx, utxo->GetTransactionHash().c_str(), -1, SQLITE_TRANSIENT)) != SQLITE_OK){
            std::cerr << "Couldn't bind TxHash" << std::endl;
            pthread_rwlock_unlock(&rwlock_);
            return false;
        }

        param_idx = sqlite3_bind_parameter_index(stmt, "@TxIndex");
        if((rc = sqlite3_bind_int(stmt, param_idx, utxo->GetIndex())) != SQLITE_OK){
            std::cerr << "Couldn't bind TxIndex" << std::endl;
            pthread_rwlock_unlock(&rwlock_);
            return false;
        }

        param_idx = sqlite3_bind_parameter_index(stmt, "@User");
        if((rc = sqlite3_bind_text(stmt, param_idx, utxo->GetUser().c_str(), -1, SQLITE_TRANSIENT)) != SQLITE_OK){
            std::cerr << "Couldn't bind User" << std::endl;
            pthread_rwlock_unlock(&rwlock_);
            return false;
        }

        param_idx = sqlite3_bind_parameter_index(stmt, "@Token");
        if((rc = sqlite3_bind_text(stmt, param_idx, utxo->GetToken().c_str(), -1, SQLITE_TRANSIENT)) != SQLITE_OK){
            std::cerr << "Couldn't bind Token" << std::endl;
            pthread_rwlock_unlock(&rwlock_);
            return false;
        }

        if((rc = sqlite3_step(stmt)) != SQLITE_DONE){
            std::cerr << "Unable to insert value:" << sqlite3_errmsg(database_) << std::endl;
            sqlite3_free(stmt);
            pthread_rwlock_unlock(&rwlock_);
            return false;
        }
        sqlite3_finalize(stmt);
        pthread_rwlock_unlock(&rwlock_);
        return true;
    }

    bool UnclaimedTransactionPool::ClearUnclaimedTransactions(){
        pthread_rwlock_wrlock(&rwlock_);
        std::string sql = "DELETE FROM UnclaimedTransactions;";

        sqlite3_stmt* stmt;
        int rc;
        if((rc = sqlite3_prepare_v2(database_, sql.c_str(), -1, &stmt, NULL)) != SQLITE_OK) {
            std::cerr << "Couldn't prepare statement: " << sqlite3_errmsg(database_) << std::endl;
            pthread_rwlock_unlock(&rwlock_);
            return false;
        }

        if((rc = sqlite3_step(stmt)) != SQLITE_DONE){
            std::cerr << "Unable to clear unclaimed transactions" << sqlite3_errmsg(database_) << std::endl;
            sqlite3_free(stmt);
            pthread_rwlock_unlock(&rwlock_);
            return false;
        }
        sqlite3_finalize(stmt);
        pthread_rwlock_unlock(&rwlock_);
        return true;
    }

    bool UnclaimedTransactionPool::LoadUnclaimedTransactionPool(const std::string &filename) {
        UnclaimedTransactionPool *instance = GetInstance();
        std::cout << "Loading utxo pool: " << filename << std::endl;
        int rc;
        if ((rc = sqlite3_open(filename.c_str(), &instance->database_)) != SQLITE_OK) {
            std::cerr << "Unable to open UnclaimedTransactionPool @" << filename << std::endl;
            sqlite3_close(instance->database_);
            return false;
        }
        std::string sql = "CREATE TABLE IF NOT EXISTS UnclaimedTransactions ("
                          "UTxHash CHAR(64) PRIMARY KEY NOT NULL,"
                          "TxHash CHAR(64) NOT NULL,"
                          "TxIndex INT NOT NULL,"
                          "User CHAR(128) NOT NULL,"
                          "Token CHAR(128) NOT NULL"
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
        Messages::UnclaimedTransaction utxo;
        utxo.set_tx_hash(GetTransactionHash());
        utxo.set_index(GetIndex());
        uint32_t size = static_cast<uint32_t>(utxo.ByteSizeLong());
        bb->Resize(size);
        utxo.SerializeToArray(bb->GetBytes(), size);
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