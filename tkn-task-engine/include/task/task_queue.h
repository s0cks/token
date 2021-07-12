#ifndef TKN_TASK_QUEUE_H
#define TKN_TASK_QUEUE_H

#include "os_thread.h"
#include "atomic/work_stealing_queue.h"

namespace token{
  namespace task{
    typedef atomic::WorkStealingQueue<uword> TaskQueue;
  }
}

#endif//TKN_TASK_QUEUE_H