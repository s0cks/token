#include "blockchain_state.h"
#include <rapidjson/writer.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/stringbuffer.h>
#include <sstream>
#include <utxo.h>

namespace Token{
    std::string BlockChainState::GetStateFile() const{
        std::stringstream stream;
        stream << GetRootDirectory() << "/state.json";
        return stream.str();
    }

    std::string BlockChainState::GetBlockFile(uint32_t idx) const{
        std::stringstream stream;
        stream << GetRootDirectory() << "/blk" << idx << ".dat";
        return stream.str();
    }

    std::string BlockChainState::GetUnclaimedTransactionPoolFile() const{
        std::stringstream stream;
        stream << GetRootDirectory() << "/unclaimed.db";
        return stream.str();
    }

    static inline bool
    FileExists(const std::string& name){
        std::ifstream f(name.c_str());
        return f.good();
    }

    void BlockChainState::LoadState(){
        std::string state_filename = GetStateFile();
        std::cout << "Loading " << state_filename << std::endl;
        if(!FileExists(state_filename)){
            std::cout << "Initializing state" << std::endl;
            return InitializeState();
        }
        std::ifstream ifs(state_filename);
        rapidjson::IStreamWrapper isw(ifs);
        doc_.ParseStream(isw);
        height_ = doc_["Height"].GetInt();
    }

    void BlockChainState::InitializeState(){
        std::ofstream ofs(GetStateFile(), std::ios::out|std::ios::trunc);
        rapidjson::OStreamWrapper osw(ofs);
        rapidjson::Writer<rapidjson::OStreamWrapper> writer(osw);
        doc_.Accept(writer);
        height_ = 0;
        SaveState();
    }

    void BlockChainState::SaveState(){
        if(!doc_.HasMember("Height")){
            rapidjson::Value height(rapidjson::kNumberType);
            doc_.AddMember("Height", height, doc_.GetAllocator());
        }
        doc_["Height"] = height_;

        std::ofstream ofs(GetStateFile(), std::ios::out|std::ios::trunc);
        rapidjson::OStreamWrapper osw(ofs);
        rapidjson::Writer<rapidjson::OStreamWrapper> writer(osw);
        doc_.Accept(writer);
    }

    BlockChainState* BlockChainState::LoadState(const std::string &root){
        BlockChainState* state = new BlockChainState(root);
        state->LoadState();
        return state;
    }

    bool BlockChainState::CanLoadStateFrom(const std::string &root){
        std::stringstream stream;
        stream << root << "/state.json";
        return FileExists(stream.str());
    }
}