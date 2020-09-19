#ifndef TOKEN_VTHREAD_H
#define TOKEN_VTHREAD_H

//TODO os-switch
#include "common.h"
#include "vthread_linux.h"
#include "handle.h"

namespace Token{
    typedef void (*ThreadHandlerFunction)(uword parameter);

    class Thread{
    public:
        enum State{
            kRunning,
            kPaused,
            kStopped,
        };
    protected:
        const ThreadId id_;
        std::string name_;

        Thread():
            id_(0),
            name_(){}

        static size_t GetMaxStackSize();
        static int Start(const char* name, ThreadHandlerFunction function, uword parameter);
        static void WriteBarrier(Object** slot, Object* data);

        template<typename T>
        static void WriteBarrier(T** slot, const Handle<T>& handle){
            WriteBarrier((Object**)slot, (Object*)handle);
        }
    public:
        virtual ~Thread() = delete;
    };
}

#endif //TOKEN_VTHREAD_H