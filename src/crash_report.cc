#include <sstream>
#include "crash_report.h"
#include "token.h"
#include "block.h"
#include "block_chain.h"
#include "unclaimed_transaction.h"
#include "transaction.h"

namespace Token{
    static inline std::string
    CreateBanner(const std::string& header){
        int middle = (CrashReport::kBannerSize - header.size() - 1) / 2;

        std::stringstream stream;
        for(int idx = 0; idx <= CrashReport::kBannerSize; idx++) stream << "#";
        stream << std::endl;
        for(int idx = 0; idx < middle; idx++) stream << " ";
        stream << header << std::endl;
        for(int idx = 0; idx < CrashReport::kBannerSize; idx++) stream << "#";
        return stream.str();
    }

    int CrashReport::WriteReport(){
        file_ << CreateBanner("Token v" + Token::GetVersion());
        file_ << std::endl;
        file_ << std::endl;
        if(ShouldPrintAllocatorInformation()) PrintAllocatorInformation();
        file_ << std::endl;
        if(ShouldPrintBlockChainInformation()) PrintBlockChainInformation();
        file_ << std::endl;
        if(ShouldPrintTransactionPoolInformation()) PrintTransactionPoolInformation();
        file_ << std::endl;
        if(ShouldPrintUnclaimedTransactionPoolInformation()) PrintUnclaimedTransactionPoolInformation();
        file_.close();
        return EXIT_FAILURE;
    }

    void CrashReport::PrintAllocatorInformation(){
        //TODO: Implement
    }

    void CrashReport::PrintBlockChainInformation(){
        file_ << "Block Chain Information:" << std::endl;

        BlockHeader head = BlockChain::GetHead();
        file_ << "  <HEAD>" << std::endl;
        file_ << "  Height: " << head.GetHeight() << std::endl;
        file_ << "  Previous Hash: " << head.GetPreviousHash() << std::endl;
        file_ << "  Merkle Root: " << head.GetMerkleRoot() << std::endl;
        file_ << "  Hash: " << head.GetHash() << std::endl;
        file_ << "  Timestamp: " << std::endl;
        file_ << "  </HEAD>" << std::endl;
    }

    void CrashReport::PrintTransactionPoolInformation(){

    }

    void CrashReport::PrintUnclaimedTransactionPoolInformation(){

    }
}