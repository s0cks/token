#include "job/worker.h"
#include "job/scheduler.h"
#include "utils/timeline.h"

namespace Token{
    Job* JobWorker::GetNextJob(){
        Job* job = queue_.Pop();
        if(job)
            return job;

        JobWorker* worker = JobScheduler::GetRandomWorker();
        if(worker == this){
            std::this_thread::yield();
            return nullptr;
        }

        if(!worker){
            std::this_thread::yield();
            return nullptr;
        }
        return worker->queue_.Steal();
    }

    void JobWorker::HandleThread(JobWorker* worker){
        LOG(INFO) << "starting worker....";
        worker->SetState(JobWorker::kRunning);

        char truncated_name[16];
        snprintf(truncated_name, 15, "Worker%" PRId16, worker->GetWorkerID());

        int result;
        if((result = pthread_setname_np(pthread_self(), truncated_name)) != 0){
            LOG(WARNING) << "couldn't set thread name: " << strerror(result);
            goto finish;
        }

        sleep(2);
        while(worker->IsRunning()){
            Job* next = worker->GetNextJob();
            if(next){
#ifdef TOKEN_DEBUG
                Histogram& histogram = worker->GetHistogram();
                Timeline timeline("RunTask");
                timeline << "Start";
#endif//TOKEN_DEBUG

                if(!next->Run()){
                    LOG(WARNING) << "couldn't run the \"" << next->GetName() << "\" job.";
                    continue;
                }

#ifdef TOKEN_DEBUG
                timeline << "Stop";
                histogram->Update(timeline.GetTotalTime());
#endif//TOKEN_DEBUG

                LOG(INFO) << next->GetName() << " has finished.";

#ifdef TOKEN_DEBUG
//                LOG(INFO) << next->GetName() << " task:";
//                LOG(INFO) << " - result: " << next->GetResult();
//                LOG(INFO) << " - start: " << GetTimestampFormattedReadable(timeline.GetStartTime());
//                LOG(INFO) << " - stop: " << GetTimestampFormattedReadable(timeline.GetStartTime());
//                LOG(INFO) << " - total time (Seconds): " << timeline.GetTotalTime();
#endif//TOKEN_DEBUG
            }
        }
    finish:
        pthread_exit(nullptr);
    }
}