#include "utils/metrics.h"

namespace Token{
    static MetricSet metrics_;
    static CounterMap counters_;
    static GaugeMap gauges_;

    bool MetricRegistry::Register(const Counter& counter){
        if(!metrics_.insert(counter).second)
            return false;
        return counters_.insert({ counter->GetName(), counter }).second;
    }

    bool MetricRegistry::Register(const Gauge& gauge){
        if(!metrics_.insert(gauge).second)
            return false;
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
            metrics_.insert(counter);
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

    bool MetricRegistry::VisitMetrics(MetricRegistryVisitor* vis){
        for(auto& it : metrics_){
            if(!vis->Visit(it))
                return false;
        }
        return true;
    }

    bool MetricRegistry::VisitGauges(MetricRegistryVisitor* vis){
        //TODO: optimize
        for(auto& it : metrics_){
            if(it->IsGauge() && !vis->Visit(it))
                return false;
        }
        return true;
    }

    bool MetricRegistry::VisitCounters(MetricRegistryVisitor* vis){
        //TODO: optimize
        for(auto& it : metrics_){
            if(it->IsCounter() && !vis->Visit(it))
                return false;
        }
        return true;
    }
}