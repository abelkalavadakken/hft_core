#pragma once

#include <chrono>
#include <cstdint>

namespace hft::core {

class Timer {
public:
    using clock_type = std::chrono::high_resolution_clock;
    using time_point = std::chrono::time_point<clock_type>;
    using nanoseconds = std::chrono::nanoseconds;
    using microseconds = std::chrono::microseconds;

    static inline uint64_t rdtsc() noexcept {
#if defined(__x86_64__) || defined(_M_X64)
        uint32_t lo, hi;
        __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
        return (static_cast<uint64_t>(hi) << 32) | lo;
#else
        // Fallback for non-x86 platforms (like Apple Silicon)
        return std::chrono::high_resolution_clock::now().time_since_epoch().count();
#endif
    }

    static inline time_point now() noexcept {
        return clock_type::now();
    }

    static inline uint64_t nanos_since_epoch() noexcept {
        return std::chrono::duration_cast<nanoseconds>(
            now().time_since_epoch()).count();
    }

    static inline uint64_t micros_since_epoch() noexcept {
        return std::chrono::duration_cast<microseconds>(
            now().time_since_epoch()).count();
    }

    static inline double tsc_to_nanos(uint64_t tsc_start, uint64_t tsc_end) noexcept {
        static const double tsc_freq = get_tsc_frequency();
        return (tsc_end - tsc_start) / tsc_freq;
    }

private:
    static double get_tsc_frequency() noexcept;
};

class ScopedTimer {
public:
    explicit ScopedTimer(uint64_t& duration_ns) 
        : duration_ref_(duration_ns), start_tsc_(Timer::rdtsc()) {}

    ~ScopedTimer() {
        duration_ref_ = static_cast<uint64_t>(
            Timer::tsc_to_nanos(start_tsc_, Timer::rdtsc()));
    }

private:
    uint64_t& duration_ref_;
    uint64_t start_tsc_;
};

} // namespace hft::core