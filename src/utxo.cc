#include <cstring>
#include <glog/logging.h>

#include "common.h"
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

    bool UnclaimedTransactionPool::GetUnclaimedTransactions(const std::string& user, std::vector<std::string>& utxos){
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
            utxos.push_back(utx_hash);
        }

        sqlite3_finalize(stmt);
        UNLOCK;
        return utxos.size() > 0;
    }

    bool UnclaimedTransactionPool::GetUnclaimedTransactions(std::vector<std::string>& utxos){
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
            utxos.push_back(utx_hash);
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

    bool UnclaimedTransactionPool::GetUnclaimedTransaction(const std::string& hash, UnclaimedTransaction* result){
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
            UnclaimedTransaction utxo(tx_hash, index, user, token);
            (*result) = utxo;
            UNLOCK;
            return true;
        }

        sqlite3_finalize(stmt);
        UNLOCK;
        return false;
    }

    bool UnclaimedTransactionPool::GetUnclaimedTransaction(const std::string& txhash, int index, UnclaimedTransaction* result){
        READ_LOCK;
        std::string sql = "SELECT UTxHash,TxHash,TxIndex,User,Token FROM UnclaimedTransactions WHERE TxHash=@TxHash AND TxIndex=@TxIndex;";

        sqlite3_stmt* stmt;
        PREPARE_SQLITE_STATEMENT;
        BIND_SQLITE_PARAMETER_TEXT("@TxHash", txhash);
        BIND_SQLITE_PARAMETER_INT("@TxIndex", index);

        int rc;
        while((rc = sqlite3_step(stmt)) == SQLITE_ROW){
            std::string utx_hash = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
            std::string tx_hash = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
            int idx = sqlite3_column_int(stmt, 2);
            std::string user = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
            std::string token = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)));
            UnclaimedTransaction utxo(tx_hash, idx, user, token);
            (*result) = utxo;
            UNLOCK;
            return true;
        }

        sqlite3_finalize(stmt);
        UNLOCK;
        return false;
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
            UNLOCK;
            return false;
        }

        sqlite3_finalize(stmt);
        UNLOCK;
        return true;
    }

    bool UnclaimedTransactionPool::AddUnclaimedTransaction(Token::UnclaimedTransaction* utxo){
        WRITE_LOCK;
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

    bool UnclaimedTransactionPool::LoadPool(const std::string& filename){
        LOG(INFO) << "loading utxo pool from: " << filename << "...";
        int rc;
        if((rc = sqlite3_open(filename.c_str(), &database_)) != SQLITE_OK){
            sqlite3_close(database_);
            LOG(ERROR) << "*** error opening utxo '" << filename << "':";
            LOG(ERROR) << "***   - SQL Error: " << sqlite3_errstr(rc);
            LOG(ERROR) << "************************";
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
            LOG(ERROR) << "*** error loading utxo pool '" << filename << "':";
            LOG(ERROR) << "***   - SQL Error: " << err;
            LOG(ERROR) << "************************";
            sqlite3_free(err);
            sqlite3_close(database_);
            return false;
        }
        LOG(INFO) << "done loading utxo pool!";
        return true;
    }

    bool UnclaimedTransactionPool::LoadUnclaimedTransactionPool(){
        return GetInstance()->LoadPool((TOKEN_BLOCKCHAIN_HOME + "/unclaimed"));
    }

    std::string UnclaimedTransaction::GetHash(){
        CryptoPP::SHA256 func;
        std::string digest;
        uint32_t size = GetRaw()->ByteSizeLong();
        uint8_t bytes[size];
        GetRaw()->SerializeToArray(bytes, size);
        CryptoPP::ArraySource source(bytes, size, true, new CryptoPP::HashFilter(func, new CryptoPP::HexEncoder(new CryptoPP::StringSink(digest))));
        return digest;
    }

    std::string UnclaimedTransaction::ToString(){
        std::stringstream stream;
        stream << "UnclaimedTransaction(" << GetHash() << ",from='" << GetUser() << "')";
        return stream.str();
    }

    UnclaimedTransaction::UnclaimedTransaction(const std::string &utxo_hash):
        raw_(){
        UnclaimedTransaction utxo;
        if(!UnclaimedTransactionPool::GetInstance()->GetUnclaimedTransaction(utxo_hash, &utxo)){
            LOG(ERROR) << "cannot get unclaimed transaction: " << utxo_hash;
            GetRaw()->Clear();
            return;
        }
        GetRaw()->CopyFrom(utxo.raw_);
    }

    bool UnclaimedTransactionPool::ClearUnclaimedTransactions(){
        //TODO: Implement
        return true;
    }

    bool UnclaimedTransactionPool::Accept(Token::UnclaimedTransactionPoolVisitor* vis){
        std::vector<std::string> utxos;
        if(!GetInstance()->GetUnclaimedTransactions(utxos)){
            LOG(ERROR) << "cannot get unclaimed transactions from pool";
            return false;
        }

        vis->VisitStart();
        for(auto& it : utxos){
            UnclaimedTransaction utx(it);
            vis->VisitUnclaimedTransaction(&utx);
        }
        vis->VisitEnd();
    }
}