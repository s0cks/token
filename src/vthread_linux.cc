#include <cstring>
#include "vthread.h"

namespace Token{
#define CHECK_RESULT(Result) if((Result) != 0) return (Result);
#define VALIDATE_RESULT(Result) \
    if((Result) != 0){ \
        LOG(WARNING) << "couldn't validate pthread result: " << (Result); \
    }

    class ThreadStartData{
    private:
        const char* name_;
        ThreadHandlerFunction function_;
        uword parameter_;
    public:
        ThreadStartData(const std::string& name, ThreadHandlerFunction function, uword parameter):
            name_(strdup(name.c_str())), // do I need to copy this?
            function_(function),
            parameter_(parameter){}
        ~ThreadStartData() = default;

        const char* GetName() const{
            return name_;
        }

        ThreadHandlerFunction GetFunction() const{
            return function_;
        }

        uword GetParameter() const{
            return parameter_;
        }
    };

    static void*
    ThreadStart(void* pdata){
        ThreadStartData* data = (ThreadStartData*)pdata;
        const char* name = data->GetName();
        ThreadHandlerFunction func = data->GetFunction();
        uword parameter = data->GetParameter();

        char truncated_name[16];
        snprintf(truncated_name, 15, "%s", name);
        pthread_setname_np(pthread_self(), truncated_name);

        func(parameter);
        return NULL;
    }

    size_t Thread::GetMaxStackSize(){
        return 128 * sizeof(uword) * 1024;
    }

    int Thread::Start(const char *name, ThreadHandlerFunction function, uword parameter){
        pthread_attr_t attr;
        int result = pthread_attr_init(&attr);
        CHECK_RESULT(result);

        result = pthread_attr_setstacksize(&attr, GetMaxStackSize());
        CHECK_RESULT(result);

        ThreadStartData* data = new ThreadStartData(name, function, parameter);

        pthread_t thid;
        result = pthread_create(&thid, &attr, &ThreadStart, data);
        CHECK_RESULT(result);

        result = pthread_attr_destroy(&attr);
        CHECK_RESULT(result);
        return 0;
    }
}