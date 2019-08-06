#include "utxo.h"
#include <cstring>

namespace Token{
    UnclaimedTransactionPool::UnclaimedTransactionPool():
        db_path_(),
        database_(nullptr){

    }

    UnclaimedTransactionPool::~UnclaimedTransactionPool(){
        sqlite3_close(database_);
    }

    UnclaimedTransactionPool* UnclaimedTransactionPool::GetInstance(){
        static UnclaimedTransactionPool instance;
        return &instance;
    }

    bool UnclaimedTransactionPool::GetUnclaimedTransactions(const std::string& user, std::vector<UnclaimedTransaction>& utxos){
        std::string sql = "SELECT * FROM Accounts;";
        char* err_msg = NULL;

        sqlite3_stmt* stmt;
        int rc;
        if((rc = sqlite3_prepare_v2(database_, sql.c_str(), -1, &stmt, NULL)) != SQLITE_OK){
            std::cerr << "Couldn't prepare statement: " << err_msg << std::endl;
            return false;
        }

        sqlite3_bind_text(stmt, 1, user.c_str(), -1, SQLITE_TRANSIENT);
        while((rc = sqlite3_step(stmt)) == SQLITE_ROW){
            std::cout << "Raw" << std::endl;
            const char* hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            int index = sqlite3_column_int(stmt, 1);
            const char* username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            const char* token = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            std::cout << "append" << std::endl;
            utxos.push_back(UnclaimedTransaction(std::string(hash), index, std::string(user), std::string(token)));
        }
        sqlite3_finalize(stmt);
        return utxos.size() > 0;
    }

    bool UnclaimedTransactionPool::AddUnclaimedTransaction(Token::UnclaimedTransaction* utxo){
        std::string sql = "INSERT INTO Accounts (TxHash, TxIndex, User, Token) VALUES (@TxHash, @TxIndex, @User, @Token);";

        sqlite3_stmt* stmt;
        int rc;
        if((rc = sqlite3_prepare_v2(database_, sql.c_str(), -1, &stmt, NULL)) != SQLITE_OK) {
            std::cerr << "Couldn't prepare statement: " << std::endl;
            return false;
        }

        int param_idx = sqlite3_bind_parameter_index(stmt, "@TxHash");
        if((rc = sqlite3_bind_text(stmt, param_idx, utxo->GetHash().c_str(), -1, SQLITE_TRANSIENT)) != SQLITE_OK){
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

    bool UnclaimedTransactionPool::LoadUnclaimedTransactionPool(const std::string &filename){
        UnclaimedTransactionPool* instance = GetInstance();
        int rc;
        if((rc = sqlite3_open(filename.c_str(), &instance->database_)) != SQLITE_OK){
            std::cerr << "Unable to open UnclaimedTransactionPool @" << filename << std::endl;
            sqlite3_close(instance->database_);
            return false;
        }
        std::string sql = "CREATE TABLE IF NOT EXISTS Accounts ("
                    "TxHash CHAR(64) PRIMARY KEY NOT NULL,"
                    "TxIndex INT NOT NULL,"
                    "User CHAR(128) NOT NULL,"
                    "Token CHAR(64) NOT NULL"
                    ");";
        char *err_msg = 0;
        if((rc = sqlite3_exec(instance->database_, sql.c_str(), NULL, NULL, &err_msg)) != SQLITE_OK){
            std::cerr << "Couldn't create accounts table:" << err_msg << std::endl;
            sqlite3_free(err_msg);
            sqlite3_close(instance->database_);
            return false;
        }
        return true;
    }
}