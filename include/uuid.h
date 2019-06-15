#ifndef TOKEN_UUID_H
#define TOKEN_UUID_H

#include <stdint.h>
#include <string>
#include <sstream>
#include <iomanip>

namespace Token{
    struct UUID{
        uint64_t ab;
        uint64_t cd;
        static UUID& v4();

        bool operator==(const UUID& other) const{
            return str() == other.str();
        }

        bool operator!=(const UUID& other) const{
            return str() != other.str();
        }

        bool operator<(const UUID& other) const{
            return str() < other.str();
        }

        std::string str() const{
            std::stringstream ss;
            ss << std::hex << std::nouppercase << std::setfill('0');
            uint32_t a = static_cast<uint32_t>(ab >> 32);
            uint32_t b = static_cast<uint32_t>(ab & 0xFFFFFFFF);
            uint32_t c = static_cast<uint32_t>(cd >> 32);
            uint32_t d = static_cast<uint32_t>(cd & 0xFFFFFFFF);
            ss << std::setw(8) << a << "-";
            ss << std::setw(4) << (b >> 16) << "-";
            ss << std::setw(4) << (b & 0xFFFF) << "-";
            ss << std::setw(4) << (c >> 16) << "-";
            ss << std::setw(4) << (c & 0xFFFF);
            ss << std::setw(8) << d;
            return ss.str();
        }

        inline friend std::ostream& operator<<(std::ostream& stream, const UUID& self){
            return stream << self.str();
        }
    };
}

namespace std{
    template<>
    class hash<Token::UUID>{
    public:
        size_t operator()(const Token::UUID& uuid) const{
            return size_t(uuid.ab ^ uuid.cd);
        }
    };
}

#endif //TOKEN_UUID_H