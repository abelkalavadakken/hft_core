#include <gtest/gtest.h>
#include "hft_core/Timer.hpp"
#include <thread>
#include <chrono>

using namespace hft::core;

class TimerTest : public ::testing::Test {};

TEST_F(TimerTest, BasicTimeFunctions) {
    auto now1 = Timer::now();
    auto nanos1 = Timer::nanos_since_epoch();
    auto micros1 = Timer::micros_since_epoch();
    
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    
    auto now2 = Timer::now();
    auto nanos2 = Timer::nanos_since_epoch();
    auto micros2 = Timer::micros_since_epoch();
    
    EXPECT_GT(now2, now1);
    EXPECT_GT(nanos2, nanos1);
    EXPECT_GT(micros2, micros1);
}

TEST_F(TimerTest, RdtscFunction) {
    uint64_t tsc1 = Timer::rdtsc();
    
    // Do some work
    volatile int sum = 0;
    for (int i = 0; i < 1000; ++i) {
        sum += i;
    }
    
    uint64_t tsc2 = Timer::rdtsc();
    
    EXPECT_GT(tsc2, tsc1);
}

TEST_F(TimerTest, TscToNanos) {
    uint64_t tsc_start = Timer::rdtsc();
    
    std::this_thread::sleep_for(std::chrono::microseconds(100));
    
    uint64_t tsc_end = Timer::rdtsc();
    
    double duration_ns = Timer::tsc_to_nanos(tsc_start, tsc_end);
    
    // Should be approximately 100 microseconds = 100,000 nanoseconds
    // Allow for some variance due to system overhead
    EXPECT_GT(duration_ns, 50000.0);  // At least 50 microseconds
    EXPECT_LT(duration_ns, 1000000.0); // Less than 1 millisecond
}

TEST_F(TimerTest, ScopedTimer) {
    uint64_t duration_ns = 0;
    
    {
        ScopedTimer timer(duration_ns);
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
    
    // Should measure approximately 100 microseconds = 100,000 nanoseconds
    EXPECT_GT(duration_ns, 50000);   // At least 50 microseconds
    EXPECT_LT(duration_ns, 1000000); // Less than 1 millisecond
}

TEST_F(TimerTest, TimeConsistency) {
    // Test that different time measurement methods are consistent
    auto start_time = Timer::now();
    uint64_t start_nanos = Timer::nanos_since_epoch();
    
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    auto end_time = Timer::now();
    uint64_t end_nanos = Timer::nanos_since_epoch();
    
    auto duration_chrono = std::chrono::duration_cast<std::chrono::nanoseconds>(
        end_time - start_time).count();
    uint64_t duration_direct = end_nanos - start_nanos;
    
    // The two measurements should be close (within 1ms tolerance)
    uint64_t diff = std::abs(static_cast<int64_t>(duration_chrono - duration_direct));
    EXPECT_LT(diff, 1000000); // Less than 1ms difference
}