#ifndef TOKEN_CONFIGURATION_H
#define TOKEN_CONFIGURATION_H

#include <libconfig.h++>

namespace Token{
    class BlockChainConfiguration{
    private:
        BlockChainConfiguration() = delete;
    public:
        ~BlockChainConfiguration() = delete;

        static void Initialize();
        static bool SaveConfiguration();
        static libconfig::Config* GetConfiguration();

        static inline libconfig::Setting& GetRoot(){
            return GetConfiguration()->getRoot();
        }

        static inline libconfig::Setting& GetProperty(const std::string& name, libconfig::Setting::Type type){
            libconfig::Setting& root = GetRoot();
            if(root.exists(name)) return root.lookup(name);
            return root.add(name, type);
        }
    };
}

#endif //TOKEN_CONFIGURATION_H