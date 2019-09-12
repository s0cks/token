#include <cstring>
#include <glog/logging.h>
#include "utxo.h"

namespace Token{
#define READ_LOCK pthread_rwlock_rdlock(&rwlock_)
#define WRITE_LOCK pthread_rwlock_wrlock(&rwlock_)
#define UNLOCK pthread_rwlock_unlock(&rwlock_)
#define PREPARE_SQLITE_STATEMENT ({ \
    int rc; \
    if((rc = sqlite3_prepare_v2(database_, sql.c_str(), -1, &stmt, NULL)) != SQLITE_OK){ \
        LOG(ERROR) << "couldn't prepare sqlite3 statement '" << sql << "': " << sqlite3_errstr(rc); \
        UNLOCK; \
        return false; \
    } \
})
#define BIND_SQLITE_PARAMETER_TEXT(Param, Value) ({ \
    int pidx = sqlite3_bind_parameter_index(stmt, Param); \
    LOG(INFO) << "binding parameter " << (Param) << "@" << pidx << " := " << (Value); \
    int rc; \
    if((rc = sqlite3_bind_text(stmt, pidx, (Value).c_str(), -1, SQLITE_TRANSIENT)) != SQLITE_OK){ \
        LOG(ERROR) << "couldn't bind parameter " << (Param) << " for statement"; \
        UNLOCK; \
        return false; \
    } \
})
#define BIND_SQLITE_PARAMETER_INT(Param, Value) ({ \
    int pidx = sqlite3_bind_parameter_index(stmt, Param); \
    int rc; \
    if((rc = sqlite3_bind_int(stmt, pidx, (Value))) != SQLITE_OK){ \
        LOG(ERROR) << "couldn't bind parameter " << (Param) << " for statement"; \
        UNLOCK; \
        return false; \
    } \
})

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

    bool UnclaimedTransactionPool::GetUnclaimedTransactions(const std::string& user, Messages::UnclaimedTransactionList* utxos){
        READ_LOCK;
        std::string sql = "SELECT UTxHash,TxHash,TxIndex,User,Token FROM UnclaimedTransactions WHERE User=@User;";

        sqlite3_stmt* stmt;
        PREPARE_SQLITE_STATEMENT;
        BIND_SQLITE_PARAMETER_TEXT("@User", user);

        int rc;
        while((rc = sqlite3_step(stmt)) == SQLITE_ROW){
            Messages::UnclaimedTransaction* utxo = utxos->add_transactions();
            utxo->set_utx_hash(std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0))));
            utxo->set_tx_hash(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
            utxo->set_index(sqlite3_column_int(stmt, 2));
            utxo->set_user(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
            utxo->set_token(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)));
        }

        sqlite3_finalize(stmt);
        UNLOCK;
        return true;
    }

    bool UnclaimedTransactionPool::GetUnclaimedTransactions(const std::string& user, std::vector<UnclaimedTransaction*>& utxos){
        READ_LOCK;
        std::string sql = "SELECT UTxHash,TxHash,TxIndex,User,Token FROM UnclaimedTransactions WHERE User=@User;";

        sqlite3_stmt* stmt;
        PREPARE_SQLITE_STATEMENT;
        BIND_SQLITE_PARAMETER_TEXT("@User", user);

        utxos.clear();
        int rc;
        while((rc = sqlite3_step(stmt)) == SQLITE_ROW){
            std::string utx_hash = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
            std::string tx_hash = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
            int index = sqlite3_column_int(stmt, 2);
            std::string u = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
            std::string token = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)));
            utxos.push_back(new UnclaimedTransaction(tx_hash, index, u, token));
        }

        sqlite3_finalize(stmt);
        UNLOCK;
        return utxos.size() > 0;
    }

    bool UnclaimedTransactionPool::GetUnclaimedTransactions(std::vector<UnclaimedTransaction*>& utxos){
        READ_LOCK;
        std::string sql = "SELECT * FROM UnclaimedTransactions;";

        sqlite3_stmt* stmt;
        PREPARE_SQLITE_STATEMENT;

        utxos.clear();
        int rc;
        while((rc = sqlite3_step(stmt)) == SQLITE_ROW){
            std::string utx_hash = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
            std::string tx_hash = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
            int index = sqlite3_column_int(stmt, 2);
            std::string user = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
            std::string token = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)));
            utxos.push_back(new UnclaimedTransaction(tx_hash, index, user, token));
        }

        sqlite3_finalize(stmt);
        UNLOCK;
        return utxos.size() > 0;
    }

    bool UnclaimedTransactionPool::GetUnclaimedTransactions(Messages::UnclaimedTransactionList* utxos){
        READ_LOCK;
        std::string sql = "SELECT UTxHash,TxHash,TxIndex,User,Token FROM UnclaimedTransactions;";

        sqlite3_stmt* stmt;
        PREPARE_SQLITE_STATEMENT;

        int rc;
        while((rc = sqlite3_step(stmt)) == SQLITE_ROW){
            Messages::UnclaimedTransaction* utxo = utxos->add_transactions();
            utxo->set_utx_hash(std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0))));
            utxo->set_tx_hash(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
            utxo->set_index(sqlite3_column_int(stmt, 2));
            utxo->set_user(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
            utxo->set_token(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)));
        }

        sqlite3_finalize(stmt);
        UNLOCK;
        return utxos->transactions_size() > 0;
    }

    bool UnclaimedTransactionPool::GetUnclaimedTransaction(const std::string& hash, UnclaimedTransaction** result){
        READ_LOCK;
        std::string sql = "SELECT UTxHash,TxHash,TxIndex,User,Token FROM UnclaimedTransactions WHERE UTxHash=@UTxHash;";

        sqlite3_stmt* stmt;
        PREPARE_SQLITE_STATEMENT;
        BIND_SQLITE_PARAMETER_TEXT("@UTxHash", hash);

        int rc;
        while((rc = sqlite3_step(stmt)) == SQLITE_ROW){
            std::string utx_hash = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
            std::string tx_hash = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
            int index = sqlite3_column_int(stmt, 2);
            std::string user = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
            std::string token = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)));
            *result = new UnclaimedTransaction(tx_hash, index, user, token);
            UNLOCK;
            return true;
        }

        sqlite3_finalize(stmt);
        UNLOCK;
        return false;
    }

    bool UnclaimedTransactionPool::GetUnclaimedTransaction(const std::string& tx_hash, int index, UnclaimedTransaction** result){
        UnclaimedTransaction utxo(tx_hash, index);
        return GetUnclaimedTransaction(utxo.GetHash(), result);
    }

    bool UnclaimedTransactionPool::RemoveUnclaimedTransaction(Token::UnclaimedTransaction* utxo){
        WRITE_LOCK;
        std::string sql = "DELETE FROM UnclaimedTransactions WHERE UTxHash=@UTxHash;";

        sqlite3_stmt* stmt;
        PREPARE_SQLITE_STATEMENT;
        BIND_SQLITE_PARAMETER_TEXT("@UTxHash", utxo->GetHash());

        int rc;
        if((rc = sqlite3_step(stmt)) != SQLITE_DONE){
            std::cout << "Unable to remove value:" << sqlite3_errmsg(database_) << std::endl;
            sqlite3_free(stmt);
            UNLOCK;
            return false;
        }

        sqlite3_finalize(stmt);
        sqlite3_free(stmt);
        UNLOCK;
        return true;
    }

    bool UnclaimedTransactionPool::AddUnclaimedTransaction(Token::UnclaimedTransaction* utxo){
        pthread_rwlock_wrlock(&rwlock_);
        std::string sql = "INSERT OR IGNORE INTO UnclaimedTransactions (UTxHash, TxHash, TxIndex, User, Token) VALUES (@UTxHash, @TxHash, @TxIndex, @User, @Token);";

        sqlite3_stmt* stmt;
        PREPARE_SQLITE_STATEMENT;
        BIND_SQLITE_PARAMETER_TEXT("@UTxHash", utxo->GetHash());
        BIND_SQLITE_PARAMETER_TEXT("@TxHash", utxo->GetTransactionHash());
        BIND_SQLITE_PARAMETER_INT("@TxIndex", utxo->GetIndex());
        BIND_SQLITE_PARAMETER_TEXT("@User", utxo->GetUser());
        BIND_SQLITE_PARAMETER_TEXT("@Token", utxo->GetToken());

        int rc;
        if((rc = sqlite3_step(stmt)) != SQLITE_DONE){
            LOG(ERROR) << "cannot insert unclaimed transaction " << utxo->GetHash();
            sqlite3_free(stmt);
            UNLOCK;
            return false;
        }

        sqlite3_finalize(stmt);
        UNLOCK;
        return true;
    }

    /*
     * TODO: Implement this function
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
    */

    bool UnclaimedTransactionPool::LoadPool(const std::string& filename){
        int rc;
        if((rc = sqlite3_open(filename.c_str(), &database_)) != SQLITE_OK){
            LOG(ERROR) << "cannot open unclaimed transaction pool: " << filename;
            sqlite3_close(database_);
            return false;
        }
        std::string sql = "CREATE TABLE IF NOT EXISTS UnclaimedTransactions ("
                              "UTxHash CHAR(64) PRIMARY KEY NOT NULL,"
                              "TxHash CHAR(64) NOT NULL,"
                              "TxIndex INT NOT NULL,"
                              "User CHAR(128) NOT NULL,"
                              "Token CHAR(128) NOT NULL"
                          ");";
        char* err;
        if((rc = sqlite3_exec(database_, sql.c_str(), NULL, NULL, &err)) != SQLITE_OK){
            LOG(ERROR) << "couldn't create the unclaimed transactions table in database: " << err;
            sqlite3_free(err);
            sqlite3_close(database_);
            return false;
        }
        return true;
    }

    bool UnclaimedTransactionPool::LoadUnclaimedTransactionPool(const std::string &filename){
        return GetInstance()->LoadPool(filename);
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

    void UnclaimedTransactionPoolPrinter::Print(){
        std::vector<UnclaimedTransaction*> utxos;
        if(!UnclaimedTransactionPool::GetInstance()->GetUnclaimedTransactions(utxos)){
            LOG(ERROR) << "can't get any unclaimed transactions";
            return;
        }
        LOG(INFO) << "*** Unclaimed Transactions:";
        int counter = 0;
        for(auto& it : utxos){
            LOG(INFO) << "***  + UnclaimedTransaction #" << (counter++) << "(" << it->GetHash() << ") := "
                    << it->GetTransactionHash() << "[" << it->GetIndex() << "]; "
                    << it->GetToken() << "(" << it->GetToken() << ")";
        }
    }
}