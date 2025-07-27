#include <gtest/gtest.h>
#include "hft_core/EventBus.hpp"
#include <atomic>
#include <thread>
#include <chrono>

using namespace hft::core;

DECLARE_EVENT(TestEvent) {
public:
    TestEvent(int value) : value_(value) {}
    int get_value() const { return value_; }
private:
    int value_;
};

DECLARE_EVENT(AnotherTestEvent) {
public:
    AnotherTestEvent(const std::string& msg) : message_(msg) {}
    const std::string& get_message() const { return message_; }
private:
    std::string message_;
};

class EventBusTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clean up any existing handlers
        auto& bus = EventBus::instance();
        bus.unsubscribe<TestEvent>();
        bus.unsubscribe<AnotherTestEvent>();
    }
};

TEST_F(EventBusTest, BasicPublishSubscribe) {
    auto& bus = EventBus::instance();
    std::atomic<int> received_value{0};
    
    bus.subscribe<TestEvent>([&received_value](const TestEvent& event) {
        received_value.store(event.get_value());
    });
    
    TestEvent event(42);
    bus.publish(event);
    
    EXPECT_EQ(received_value.load(), 42);
}

TEST_F(EventBusTest, MultipleSubscribers) {
    auto& bus = EventBus::instance();
    std::atomic<int> count{0};
    
    bus.subscribe<TestEvent>([&count](const TestEvent& event) {
        count.fetch_add(1);
    });
    
    bus.subscribe<TestEvent>([&count](const TestEvent& event) {
        count.fetch_add(1);
    });
    
    TestEvent event(1);
    bus.publish(event);
    
    EXPECT_EQ(count.load(), 2);
}

TEST_F(EventBusTest, DifferentEventTypes) {
    auto& bus = EventBus::instance();
    std::atomic<int> test_count{0};
    std::atomic<int> another_count{0};
    
    bus.subscribe<TestEvent>([&test_count](const TestEvent& event) {
        test_count.fetch_add(1);
    });
    
    bus.subscribe<AnotherTestEvent>([&another_count](const AnotherTestEvent& event) {
        another_count.fetch_add(1);
    });
    
    TestEvent test_event(1);
    AnotherTestEvent another_event("test");
    
    bus.publish(test_event);
    bus.publish(another_event);
    
    EXPECT_EQ(test_count.load(), 1);
    EXPECT_EQ(another_count.load(), 1);
}

TEST_F(EventBusTest, EmitMethod) {
    auto& bus = EventBus::instance();
    std::atomic<int> received_value{0};
    
    bus.subscribe<TestEvent>([&received_value](const TestEvent& event) {
        received_value.store(event.get_value());
    });
    
    bus.emit<TestEvent>(123);
    
    EXPECT_EQ(received_value.load(), 123);
}

TEST_F(EventBusTest, AsyncMode) {
    auto& bus = EventBus::instance();
    std::atomic<int> received_value{0};
    
    bus.subscribe<TestEvent>([&received_value](const TestEvent& event) {
        received_value.store(event.get_value());
    });
    
    bus.set_async_mode(true);
    
    TestEvent event(999);
    bus.publish(event);
    
    // Wait a bit for async processing
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    EXPECT_EQ(received_value.load(), 999);
    
    bus.set_async_mode(false);
}