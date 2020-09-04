#include "common.h"
#include "configuration.h"
#include "crash_report.h"

namespace Token{
    libconfig::Config* BlockChainConfiguration::GetConfiguration(){
        static libconfig::Config instance;
        return &instance;
    }

    static inline std::string
    GetConfigurationFilename(){
        return FLAGS_path + "/chain.cfg";
    }

    void BlockChainConfiguration::Initialize(){
        std::string filename = GetConfigurationFilename();
        try{ // I hate try-catch in C++
            if(FileExists(filename)) GetConfiguration()->readFile(filename.c_str());
        } catch(const libconfig::ParseException& exc){
            LOG(WARNING) << "failed to parse configuration from file " << filename << ": " << exc.what();
        } catch(const libconfig::FileIOException& exc){
            LOG(WARNING) << "failed to load configuration from file " << filename << ": " << exc.what();
        }
    }

    void BlockChainConfiguration::SaveConfiguration(){
        std::string filename = GetConfigurationFilename();
        try{
            GetConfiguration()->writeFile(filename.c_str());
        } catch(const libconfig::FileIOException& exc){
            LOG(WARNING) << "failed to save configuration to file " << filename << ": " << exc.what();
        }
    }
}