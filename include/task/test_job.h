#ifndef TOKEN_TEST_JOB_H
#define TOKEN_TEST_JOB_H

#include "job.h"

namespace Token{
    class TestJob;
    class WorkerJob : public Job{
        friend class TestJob;
    private:
        WorkerJob(TestJob* job, const std::string& name);
    protected:
        JobResult DoWork(){
            LOG(INFO) << GetName() << " doing work.....";
            sleep(3);
            return Success("Done!");
        }
    public:
        ~WorkerJob() = default;

        std::string ToString() const{
            return "WorkerJob()";
        }
    };

    class TestJob : public Job{
    public:
        static const int kNumberOfWorkers = 10;
    protected:
        JobResult DoWork(){
            for(int idx = 0; idx < kNumberOfWorkers; idx++){
                LOG(INFO) << "spawning worker (" << idx << "/" << kNumberOfWorkers << ")....";

                std::stringstream ss;
                ss << "worker-" << idx;
                WorkerJob* job = new WorkerJob(this, ss.str());
                if(!JobPool::Schedule(job)){
                    LOG(ERROR) << "couldn't schedule worker job.";
                    return Failed("Couldn't schedule worker job.");
                }
            }

            return Success("Done!");
        }
    public:
        TestJob(Job* parent):
            Job(parent, "Test"){}
        TestJob():
            Job(nullptr, "Test"){}
        ~TestJob() = default;

        std::string ToString() const{
            return "TestJob()";
        }
    };

    WorkerJob::WorkerJob(TestJob* job, const std::string& name):
        Job(job, name){}
}

#endif //TOKEN_TEST_JOB_H