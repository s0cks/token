#include "utils/metrics.h"

namespace Token{
    static CounterMap counters_;

    bool MetricRegistry::Register(const Counter& counter){
        return counters_.insert({ counter->GetName(), counter }).second;
    }

    Counter MetricRegistry::GetCounter(const std::string& name){
        auto pos = counters_.find(name);
        if(pos == counters_.end())
            return Counter();
        return pos->second;
    }
}