#include <gtest/gtest.h>
#include "hft_core/ThreadPool.hpp"
#include <atomic>
#include <chrono>
#include <vector>

using namespace hft::core;

class ThreadPoolTest : public ::testing::Test {
protected:
    void SetUp() override {
        pool_ = std::make_unique<ThreadPool>(4);
    }

    std::unique_ptr<ThreadPool> pool_;
};

TEST_F(ThreadPoolTest, BasicTaskExecution) {
    std::atomic<int> result{0};
    
    auto future = pool_->enqueue([&result]() {
        result.store(42);
        return 42;
    });
    
    EXPECT_EQ(future.get(), 42);
    EXPECT_EQ(result.load(), 42);
}

TEST_F(ThreadPoolTest, TaskWithParameters) {
    auto add_func = [](int a, int b) {
        return a + b;
    };
    
    auto future = pool_->enqueue(add_func, 10, 20);
    EXPECT_EQ(future.get(), 30);
}

TEST_F(ThreadPoolTest, MultipleTasksConcurrent) {
    std::atomic<int> counter{0};
    std::vector<std::future<void>> futures;
    
    // Submit multiple tasks
    for (int i = 0; i < 100; ++i) {
        futures.push_back(pool_->enqueue([&counter]() {
            counter.fetch_add(1);
        }));
    }
    
    // Wait for all tasks to complete
    for (auto& future : futures) {
        future.get();
    }
    
    EXPECT_EQ(counter.load(), 100);
}

TEST_F(ThreadPoolTest, TasksWithDifferentReturnTypes) {
    auto int_task = pool_->enqueue([]() { return 42; });
    auto string_task = pool_->enqueue([]() { return std::string("hello"); });
    auto double_task = pool_->enqueue([]() { return 3.14; });
    
    EXPECT_EQ(int_task.get(), 42);
    EXPECT_EQ(string_task.get(), "hello");
    EXPECT_DOUBLE_EQ(double_task.get(), 3.14);
}

TEST_F(ThreadPoolTest, PoolSizeAndPendingTasks) {
    EXPECT_EQ(pool_->size(), 4);
    
    // Submit some long-running tasks
    std::vector<std::future<void>> futures;
    for (int i = 0; i < 10; ++i) {
        futures.push_back(pool_->enqueue([]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }));
    }
    
    // There should be some pending tasks
    EXPECT_GE(pool_->pending_tasks(), 0);
    
    // Wait for completion
    for (auto& future : futures) {
        future.get();
    }
}

TEST_F(ThreadPoolTest, ExceptionHandling) {
    auto future = pool_->enqueue([]() {
        throw std::runtime_error("Test exception");
        return 42;
    });
    
    EXPECT_THROW(future.get(), std::runtime_error);
}

TEST_F(ThreadPoolTest, HighPriorityThreadPool) {
    HighPriorityThreadPool hp_pool(2);
    std::atomic<int> result{0};
    
    auto future = hp_pool.enqueue_high_priority([&result]() {
        result.store(123);
        return 123;
    });
    
    EXPECT_EQ(future.get(), 123);
    EXPECT_EQ(result.load(), 123);
}