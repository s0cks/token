#ifndef TOKEN_SESSION_H
#define TOKEN_SESSION_H

#include "message.h"

namespace Token{
    class Session{
    public:
        virtual ~Session() = default;

        virtual void Send(const Message& msg) = 0;
    };
}

#endif //TOKEN_SESSION_H