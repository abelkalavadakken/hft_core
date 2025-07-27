#include "hft_core/Config.hpp"
#include "hft_core/EventBus.hpp"
#include "hft_core/Logger.hpp"
#include "hft_core/MemoryPool.hpp"
#include "hft_core/ThreadPool.hpp"
#include "hft_core/Timer.hpp"

#include <iostream>
#include <chrono>
#include <thread>

using namespace hft::core;

// Define a custom event
DECLARE_EVENT(TradeEvent) {
public:
    TradeEvent(const std::string& symbol, double price, int quantity) 
        : symbol_(symbol), price_(price), quantity_(quantity) {}
    
    const std::string& symbol() const { return symbol_; }
    double price() const { return price_; }
    int quantity() const { return quantity_; }
    
private:
    std::string symbol_;
    double price_;
    int quantity_;
};

struct Order {
    std::string symbol;
    double price;
    int quantity;
    
    Order(const std::string& s, double p, int q) : symbol(s), price(p), quantity(q) {}
};

int main() {
    std::cout << "=== HFT Core Library Integration Test ===" << std::endl;
    
    // 1. Configuration Test
    std::cout << "\n1. Testing Configuration..." << std::endl;
    auto& config = Config::instance();
    config.set("test.threads", 4);
    config.set("test.latency_limit", 100.0);
    config.set("test.enabled", true);
    
    std::cout << "  Threads: " << config.get<int>("test.threads") << std::endl;
    std::cout << "  Latency limit: " << config.get<double>("test.latency_limit") << "us" << std::endl;
    std::cout << "  Enabled: " << (config.get<bool>("test.enabled") ? "Yes" : "No") << std::endl;
    
    // 2. Logger Test
    std::cout << "\n2. Testing Logger..." << std::endl;
    Logger::instance().set_level(LogLevel::INFO);
    LOG_INFO("HFT Core library initialized successfully");
    LOG_WARN("This is a warning message");
    
    // 3. EventBus Test
    std::cout << "\n3. Testing EventBus..." << std::endl;
    int trade_count = 0;
    EventBus::instance().subscribe<TradeEvent>([&trade_count](const TradeEvent& event) {
        trade_count++;
        std::cout << "  Trade received: " << event.symbol() 
                  << " @ " << event.price() 
                  << " x " << event.quantity() << std::endl;
    });
    
    EventBus::instance().emit<TradeEvent>("AAPL", 150.25, 1000);
    EventBus::instance().emit<TradeEvent>("GOOGL", 2800.50, 500);
    
    // 4. Memory Pool Test
    std::cout << "\n4. Testing Memory Pool..." << std::endl;
    MemoryPool<Order> order_pool;
    
    std::vector<Order*> orders;
    for (int i = 0; i < 5; ++i) {
        Order* order = order_pool.construct("SYMBOL" + std::to_string(i), 100.0 + i, 100 * (i + 1));
        orders.push_back(order);
        std::cout << "  Created order: " << order->symbol 
                  << " @ " << order->price 
                  << " x " << order->quantity << std::endl;
    }
    
    // Clean up orders
    for (auto* order : orders) {
        order_pool.destroy(order);
    }
    
    // 5. Thread Pool Test
    std::cout << "\n5. Testing Thread Pool..." << std::endl;
    ThreadPool pool(2);
    
    auto future1 = pool.enqueue([](int x, int y) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        return x + y;
    }, 10, 20);
    
    auto future2 = pool.enqueue([](const std::string& msg) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        return "Processed: " + msg;
    }, "Market Data");
    
    std::cout << "  Task 1 result: " << future1.get() << std::endl;
    std::cout << "  Task 2 result: " << future2.get() << std::endl;
    
    // 6. Timer Test
    std::cout << "\n6. Testing Timer..." << std::endl;
    
    uint64_t duration_ns = 0;
    {
        ScopedTimer timer(duration_ns);
        // Simulate some work
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
    
    std::cout << "  Measured execution time: " << duration_ns << " nanoseconds" << std::endl;
    std::cout << "  High-resolution timestamp: " << Timer::nanos_since_epoch() << " ns since epoch" << std::endl;
    
    // 7. Performance metrics
    std::cout << "\n7. Performance Summary..." << std::endl;
    std::cout << "  Total trades processed: " << trade_count << std::endl;
    std::cout << "  Memory pool capacity: " << order_pool.capacity() << " objects" << std::endl;
    std::cout << "  Thread pool size: " << pool.size() << " threads" << std::endl;
    
    LOG_INFO("Integration test completed successfully");
    
    std::cout << "\n=== Test Completed Successfully ===" << std::endl;
    
    // Give logger time to flush
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    return 0;
}