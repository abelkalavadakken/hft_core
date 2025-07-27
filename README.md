# hft_core

A lightweight, modular C++17 infrastructure library for building high-performance trading systems and low-latency applications.

Built during my deep dive into system-level performance and algorithmic trading infrastructure. `hft_core` offers fast, plug-and-play components that form the backbone of modern HFT engines, from thread pools to memory pools, event buses to high-resolution timers.

---

## Features

- Config – Thread-safe singleton config loader with runtime overrides
- Logger – Asynchronous logging with file/console output and level control
- ThreadPool – High-performance thread pools with task queue and clean shutdown
- EventBus – Simple and efficient pub-sub messaging (synchronous or async)
- MemoryPool – Fixed-size memory pools for allocation-free trading paths
- Timer – Nanosecond timers and TSC-based performance profiling



### Architecture Overview

```
       +----------------+
       |    Config      |
       +----------------+
               |
+----------+  +----------+   +--------------+   +---------+
|  Logger  |  | EventBus |   |  ThreadPool  |   |  Timer  |
+----------+  +----------+   +--------------+   +---------+
               |
        +----------------------+
        |   Trading System     |
        | (Strategies, Feeds)  |
        +----------------------+
               |
        +----------------------+
        |     MemoryPool       |
        +----------------------+
```
---

## Getting Started

### Requirements

- C++17 or newer
- CMake 3.16+
- Linux/macOS (uses `pthread`)
- GoogleTest (auto-downloaded for tests)

### Build

```bash
git clone https://github.com/yourusername/hft_core.git
cd hft_core
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Install (optional)

```bash
sudo make install
```

---

## Integration

### Option 1: add_subdirectory()

```cmake
add_subdirectory(path/to/hft_core)
target_link_libraries(my_target PRIVATE hft_core)
```

### Option 2: FetchContent

```cmake
include(FetchContent)
FetchContent_Declare(
    hft_core
    GIT_REPOSITORY https://github.com/yourusername/hft_core.git
    GIT_TAG main
)
FetchContent_MakeAvailable(hft_core)
target_link_libraries(my_target PRIVATE hft_core)
```

---

## Code Examples

### Configuration

```cpp
Config::instance().load_from_file("config.conf");
auto host = CONFIG_GET_STRING("server.host", "localhost");
CONFIG_SET("runtime.threads", 8);
```

### Event System

```cpp
DECLARE_EVENT(MyEvent) {
  int value;
  explicit MyEvent(int v) : value(v) {}
};

EventBus::instance().subscribe<MyEvent>([](const MyEvent& e) {
  std::cout << "Event received: " << e.value << "\n";
});

EventBus::instance().emit<MyEvent>(42);
```

### Logging

```cpp
Logger::instance().set_output_file("trade.log");
Logger::instance().set_level(LogLevel::DEBUG);
LOG_INFO("Strategy started");
LOG_ERROR("Order rejected");
```

### Thread Pool

```cpp
ThreadPool pool(4);
auto future = pool.enqueue([] { return 21 * 2; });
std::cout << future.get() << "\n";  // 42
```

### Memory Pool

```cpp
MemoryPool<Order> pool;
Order* o = pool.construct("AAPL", 100.0, 10);
pool.destroy(o);
```

### Timer

```cpp
uint64_t elapsed = 0;
{
  ScopedTimer timer(elapsed);
  // Critical code block...
}
std::cout << "Elapsed (ns): " << elapsed << "\n";
```

---

## Running Tests

```bash
cd build
ctest --output-on-failure
```

---

## Performance-Oriented Design

- Zero-heap steady-state operation
- Minimal allocations in critical paths
- Clean shutdowns and exception safety
- TSC-based latency profiling
- Easy to extend with custom components

---

## License

Feel free to use.

Built as a personal project.

---

## Author

Abel K James  
C++ Systems Developer | Performance Engineering  
[GitHub](https://github.com/abelkalavadakken) · [LinkedIn]([https://linkedin.com/in/abelkjames](https://www.linkedin.com/in/abel-kalavadakken-b37b495b/)
