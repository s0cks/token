#include "utils/metrics.h"

namespace Token{
    static MetricSet metrics_;
    static CounterMap counters_;
    static GaugeMap gauges_;
    static HistogramMap histograms_;

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

    bool MetricRegistry::Register(const Histogram& histogram){
        if(!metrics_.insert(histogram).second)
            return false;
        return histograms_.insert({ histogram->GetName(), histogram }).second;
    }

    bool MetricRegistry::HasCounter(const std::string& name){
        auto pos = counters_.find(name);
        return pos != counters_.end();
    }

    bool MetricRegistry::HasGauge(const std::string& name){
        auto pos = gauges_.find(name);
        return pos != gauges_.end();
    }

    bool MetricRegistry::HasHistogram(const std::string& name){
        auto pos = histograms_.find(name);
        return pos != histograms_.end();
    }

    Counter MetricRegistry::GetCounter(const std::string& name){
        auto pos = counters_.find(name);
        if(pos == counters_.end())
            return Counter(nullptr);
        return pos->second;
    }

    Gauge MetricRegistry::GetGauge(const std::string& name){
        auto pos = gauges_.find(name);
        if(pos == gauges_.end())
            return Gauge(nullptr);
        return pos->second;
    }

    Histogram MetricRegistry::GetHistogram(const std::string& name){
        auto pos = histograms_.find(name);
        if(pos == histograms_.end())
            return Histogram(nullptr);
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

    bool MetricRegistry::VisitHistograms(MetricRegistryVisitor* vis){
        //TODO: optimize
        for(auto& it : metrics_){
            if(it->IsHistogram() && !vis->Visit(it))
                return false;
        }
        return true;
    }
}