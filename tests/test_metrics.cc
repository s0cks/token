#include "test_suite.h"
#include "utils/metrics.h"

namespace Token{
    TEST(TestMetrics, test_counter){
        Counter counter = NewCounter("Test");
        counter->Increment();
        ASSERT_EQ(counter->Get(), 1);
        counter->Increment(2);
        ASSERT_EQ(counter->Get(), 3);
        counter->Decrement();
        ASSERT_EQ(counter->Get(), 2);
        counter->Decrement(2);
        ASSERT_EQ(counter->Get(), 0);
    }

    TEST(TestMetrics, test_registry){
        Counter counter = NewCounter("Test");
        ASSERT_TRUE(MetricRegistry::Register(counter));
        MetricRegistry::GetCounter("Test")->Increment(10);
        ASSERT_EQ(counter->Get(), 10);
    }
}