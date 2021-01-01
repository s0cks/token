#include <cstring>
#include "vthread.h"

namespace Token{
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
      if(name_)
        free((void*) name_);
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

  static void*
  HandleThread(void* pdata){
    ThreadStartData* data = (ThreadStartData*) pdata;
    ThreadHandlerFunction func = data->GetFunction();
    uword parameter = data->GetParameter();

    char truncated_name[16];
    snprintf(truncated_name, 15, "%s", data->GetName());

    int result;
    if((result = pthread_setname_np(pthread_self(), truncated_name)) != 0){
      LOG(WARNING) << "couldn't set thread name: " << strerror(result);
      goto exit;
    }

    func(parameter);
    exit:
    delete data;
    pthread_exit(NULL);
  }

  bool Thread::Start(ThreadId* thread, const char* name, ThreadHandlerFunction function, uword parameter){
    int result;
    pthread_attr_t attrs;
    if((result = pthread_attr_init(&attrs)) != 0){
      LOG(WARNING) << "couldn't initialize the thread attributes: " << strerror(result);
      return false;
    }

    ThreadStartData* data = new ThreadStartData(name, function, parameter);
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

  bool Thread::Stop(ThreadId thread){
    int result;
    if((result = pthread_join(thread, NULL)) != 0){
      LOG(WARNING) << "couldn't join thread: " << strerror(result);
      return false;
    }
    return true;
  }
}