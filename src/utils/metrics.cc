#include "utils/metrics.h"

namespace Token{
    static CounterMap counters_;
    static GaugeMap gauges_;

    bool MetricRegistry::Register(const Counter& counter){
        return counters_.insert({ counter->GetName(), counter }).second;
    }

    bool MetricRegistry::Register(const Gauge& gauge){
        return gauges_.insert({ gauge->GetName(), gauge }).second;
    }

    bool MetricRegistry::HasCounter(const std::string& name){
        auto pos = counters_.find(name);
        return pos != counters_.end();
    }

    bool MetricRegistry::HasGauge(const std::string& name){
        auto pos = gauges_.find(name);
        return pos != gauges_.end();
    }

    Counter MetricRegistry::GetCounter(const std::string& name){
        auto pos = counters_.find(name);
        if(pos == counters_.end()){
            Counter counter = std::make_shared<Metrics::Counter>(name);
            if(!counters_.insert({ name, counter }).second)
                return Counter(nullptr);
            return counter;
        }
        return pos->second;
    }

    Gauge MetricRegistry::GetGauge(const std::string& name){
        auto pos = gauges_.find(name);
        if(pos == gauges_.end())
            return Gauge(nullptr);
        return pos->second;
    }
}