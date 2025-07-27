#include "hft_core/Timer.hpp"
#include <fstream>
#include <thread>

namespace hft::core {

double Timer::get_tsc_frequency() noexcept {
    // Try to read from /proc/cpuinfo first (Linux)
    std::ifstream cpuinfo("/proc/cpuinfo");
    if (cpuinfo.is_open()) {
        std::string line;
        while (std::getline(cpuinfo, line)) {
            if (line.find("cpu MHz") != std::string::npos) {
                size_t pos = line.find(':');
                if (pos != std::string::npos) {
                    double mhz = std::stod(line.substr(pos + 1));
                    return mhz * 1e6; // Convert MHz to Hz
                }
            }
        }
    }

    // Fallback: calibrate TSC frequency
    auto start_time = std::chrono::high_resolution_clock::now();
    uint64_t start_tsc = rdtsc();
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    auto end_time = std::chrono::high_resolution_clock::now();
    uint64_t end_tsc = rdtsc();
    
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(
        end_time - start_time).count();
    
    return static_cast<double>(end_tsc - start_tsc) / duration;
}

} // namespace hft::core