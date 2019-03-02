#include "tx_handler.h"

namespace Token{
    bool TransactionHandler::HandleTransactions(Token::Array<Token::Transaction *> possibles,
                                                Token::Array<Token::Transaction *> valid){
        Array<Transaction*> list(0xA);

        for(size_t i = 0; i < possibles.Length(); i++){
            Transaction* possible = possibles[i];
            if(IsValid(possible)){
                for(int o = 0; o < possible->GetNumberOfOutputs(); o++){
                    new UnclaimedTransaction(GetUnclaimedTransactionPool(), possible, o);
                }
                for(int in = 0; in < possible->GetNumberOfInputs(); in++){
                    std::string hash = possible->GetInputAt(in).previous_hash();
                    int index = possible->GetInputAt(in).output_index();
                    GetUnclaimedTransactionPool()->Remove(hash);
                }
            }
        }
    }
}