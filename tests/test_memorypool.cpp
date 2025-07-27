#include <gtest/gtest.h>
#include "hft_core/MemoryPool.hpp"
#include <vector>

using namespace hft::core;

struct TestObject {
    int value;
    double data;
    
    TestObject(int v = 0, double d = 0.0) : value(v), data(d) {}
};

class MemoryPoolTest : public ::testing::Test {
protected:
    void SetUp() override {
        pool_ = std::make_unique<MemoryPool<TestObject>>();
    }

    std::unique_ptr<MemoryPool<TestObject>> pool_;
};

TEST_F(MemoryPoolTest, BasicAllocation) {
    TestObject* obj = pool_->allocate();
    EXPECT_NE(obj, nullptr);
    
    pool_->deallocate(obj);
}

TEST_F(MemoryPoolTest, ConstructDestroy) {
    TestObject* obj = pool_->construct(42, 3.14);
    EXPECT_NE(obj, nullptr);
    EXPECT_EQ(obj->value, 42);
    EXPECT_DOUBLE_EQ(obj->data, 3.14);
    
    pool_->destroy(obj);
}

TEST_F(MemoryPoolTest, MultipleAllocations) {
    std::vector<TestObject*> objects;
    
    // Allocate multiple objects
    for (int i = 0; i < 100; ++i) {
        TestObject* obj = pool_->construct(i, i * 2.0);
        EXPECT_NE(obj, nullptr);
        EXPECT_EQ(obj->value, i);
        objects.push_back(obj);
    }
    
    // Verify all objects are valid
    for (size_t i = 0; i < objects.size(); ++i) {
        EXPECT_EQ(objects[i]->value, static_cast<int>(i));
        EXPECT_DOUBLE_EQ(objects[i]->data, i * 2.0);
    }
    
    // Clean up
    for (auto* obj : objects) {
        pool_->destroy(obj);
    }
}

TEST_F(MemoryPoolTest, CapacityTracking) {
    size_t initial_capacity = pool_->capacity();
    size_t initial_available = pool_->available();
    
    EXPECT_GT(initial_capacity, 0);
    EXPECT_EQ(initial_available, initial_capacity);
    
    // Allocate some objects
    std::vector<TestObject*> objects;
    for (int i = 0; i < 5; ++i) {
        objects.push_back(pool_->allocate());
    }
    
    EXPECT_EQ(pool_->available(), initial_available - 5);
    
    // Deallocate
    for (auto* obj : objects) {
        pool_->deallocate(obj);
    }
    
    EXPECT_EQ(pool_->available(), initial_available);
}

TEST_F(MemoryPoolTest, LockFreeMemoryPool) {
    LockFreeMemoryPool<TestObject> lock_free_pool(10);
    
    TestObject* obj1 = lock_free_pool.allocate();
    TestObject* obj2 = lock_free_pool.allocate();
    
    EXPECT_NE(obj1, nullptr);
    EXPECT_NE(obj2, nullptr);
    EXPECT_NE(obj1, obj2);
    
    // Construct objects manually since LockFreeMemoryPool doesn't have construct method
    new (obj1) TestObject(100, 1.5);
    new (obj2) TestObject(200, 2.5);
    
    EXPECT_EQ(obj1->value, 100);
    EXPECT_EQ(obj2->value, 200);
    
    // Destruct and deallocate
    obj1->~TestObject();
    obj2->~TestObject();
    
    lock_free_pool.deallocate(obj1);
    lock_free_pool.deallocate(obj2);
}