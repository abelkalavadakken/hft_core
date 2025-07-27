#pragma once

#include <memory>
#include <vector>
#include <atomic>
#include <cstddef>
#include <new>
#include <cstdlib>

namespace hft::core {

template<typename T, size_t BlockSize = 4096>
class MemoryPool {
public:
    explicit MemoryPool(size_t initial_blocks = 1) {
        allocate_block();
    }

    ~MemoryPool() {
        for (auto* block : blocks_) {
            std::free(block);
        }
    }

    T* allocate() {
        if (free_list_.empty()) {
            allocate_block();
        }
        
        T* ptr = free_list_.back();
        free_list_.pop_back();
        return ptr;
    }

    void deallocate(T* ptr) noexcept {
        if (ptr) {
            free_list_.push_back(ptr);
        }
    }

    template<typename... Args>
    T* construct(Args&&... args) {
        T* ptr = allocate();
        try {
            new (ptr) T(std::forward<Args>(args)...);
            return ptr;
        } catch (...) {
            deallocate(ptr);
            throw;
        }
    }

    void destroy(T* ptr) noexcept {
        if (ptr) {
            ptr->~T();
            deallocate(ptr);
        }
    }

    size_t capacity() const noexcept {
        return blocks_.size() * objects_per_block_;
    }

    size_t available() const noexcept {
        return free_list_.size();
    }

private:
    static constexpr size_t objects_per_block_ = BlockSize / sizeof(T);
    
    std::vector<void*> blocks_;
    std::vector<T*> free_list_;

    void allocate_block() {
        void* block = std::aligned_alloc(alignof(T), BlockSize);
        if (!block) {
            throw std::bad_alloc{};
        }
        
        blocks_.push_back(block);
        
        T* typed_block = static_cast<T*>(block);
        for (size_t i = 0; i < objects_per_block_; ++i) {
            free_list_.push_back(typed_block + i);
        }
    }
};

template<typename T>
class LockFreeMemoryPool {
public:
    struct alignas(64) Node {
        std::atomic<Node*> next{nullptr};
        alignas(T) char data[sizeof(T)];
    };

    explicit LockFreeMemoryPool(size_t initial_size = 1000) {
        for (size_t i = 0; i < initial_size; ++i) {
            auto* node = new Node;
            Node* expected = head_.load(std::memory_order_relaxed);
            node->next.store(expected, std::memory_order_relaxed);
            while (!head_.compare_exchange_weak(
                expected, node, std::memory_order_release, std::memory_order_relaxed)) {
                node->next.store(expected, std::memory_order_relaxed);
            }
        }
    }

    ~LockFreeMemoryPool() {
        while (auto* node = head_.load()) {
            head_.store(node->next.load());
            delete node;
        }
    }

    T* allocate() {
        Node* old_head = head_.load(std::memory_order_acquire);
        while (old_head && !head_.compare_exchange_weak(
            old_head, old_head->next.load(), std::memory_order_release, std::memory_order_acquire)) {
        }
        
        if (!old_head) {
            old_head = new Node;
        }
        
        return reinterpret_cast<T*>(old_head->data);
    }

    void deallocate(T* ptr) noexcept {
        if (!ptr) return;
        
        Node* node = reinterpret_cast<Node*>(
            reinterpret_cast<char*>(ptr) - offsetof(Node, data));
        
        Node* old_head = head_.load(std::memory_order_relaxed);
        do {
            node->next.store(old_head, std::memory_order_relaxed);
        } while (!head_.compare_exchange_weak(
            old_head, node, std::memory_order_release, std::memory_order_relaxed));
    }

private:
    std::atomic<Node*> head_{nullptr};
};

} // namespace hft::core