#include <cstring>
#include "vthread.h"

namespace token{
  class ThreadStartData{
   private:
    const char* name_;
    ThreadHandlerFunction function_;
    uword parameter_;
   public:
    ThreadStartData(const char* name, ThreadHandlerFunction function, uword parameter):
      name_(strdup(name)),
      function_(function),
      parameter_(parameter){}
    ~ThreadStartData(){
      if(name_){
        free((void*) name_);
      }
    }

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

  const int8_t kMaxThreadNameSize = 16;

  bool SetThreadName(const ThreadId& thread, const char* name){
    char truncated_name[kMaxThreadNameSize];
    snprintf(truncated_name, kMaxThreadNameSize-1, "%s", name);
    int result;
    if((result = pthread_setname_np(thread, truncated_name)) != 0){
      LOG(WARNING) << "couldn't set thread name: " << strerror(result);
      return false;
    }
    return true;
  }

  static void*
  HandleThread(void* pdata){
    ThreadStartData* data = (ThreadStartData*)pdata;
    ThreadHandlerFunction func = data->GetFunction();
    uword parameter = data->GetParameter();

    if(!SetThreadName(pthread_self(), data->GetName()) != 0){
      goto exit;
    }

    func(parameter);
  exit:
    delete data;
    pthread_exit(NULL);
  }

  bool ThreadStart(ThreadId* thread, const char* name, ThreadHandlerFunction func, uword parameter){
    int result;
    pthread_attr_t attrs;
    if((result = pthread_attr_init(&attrs)) != 0){
      LOG(WARNING) << "couldn't initialize the thread attributes: " << strerror(result);
      return false;
    }

    ThreadStartData* data = new ThreadStartData(name, func, parameter);
    if((result = pthread_create(thread, &attrs, &HandleThread, data)) != 0){
      LOG(WARNING) << "couldn't start the thread: " << strerror(result);
      return false;
    }

    if((result = pthread_attr_destroy(&attrs)) != 0){
      LOG(WARNING) << "couldn't destroy the thread attributes: " << strerror(result);
      return false;
    }
    return true;
  }

  std::string GetThreadName(const ThreadId& thread){
    char name[kMaxThreadNameSize];

    int err;
    if((err = pthread_getname_np(thread, name, kMaxThreadNameSize)) != 0){
      LOG(WARNING) << "cannot get name for " << thread << " thread: " << strerror(err);
      return "unknown";
    }
    return std::string(name);
  }

  bool ThreadJoin(const ThreadId& thread){
    int result;
    if((result = pthread_join(thread, NULL)) != 0){
      LOG(WARNING) << "couldn't join thread: " << strerror(result);
      return false;
    }
    return true;
  }

  ThreadId GetCurrentThread(){
    return pthread_self();
  }
}