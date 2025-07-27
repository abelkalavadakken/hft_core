#pragma once

#include <functional>
#include <vector>
#include <unordered_map>
#include <typeindex>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <queue>
#include <thread>
#include <condition_variable>
#include <chrono>

namespace hft::core {

class Event {
public:
    virtual ~Event() = default;
    uint64_t timestamp = 0;
    std::type_index type_id;
    
protected:
    Event(std::type_index tid) : type_id(tid) {
        timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    }
};

template<typename T>
class TypedEvent : public Event {
public:
    TypedEvent() : Event(std::type_index(typeid(T))) {}
};

class IEventHandler {
public:
    virtual ~IEventHandler() = default;
    virtual void handle(const Event& event) = 0;
    virtual std::type_index get_event_type() const = 0;
};

template<typename EventType>
class EventHandler : public IEventHandler {
public:
    using HandlerFunc = std::function<void(const EventType&)>;
    
    explicit EventHandler(HandlerFunc handler) : handler_(std::move(handler)) {}
    
    void handle(const Event& event) override {
        handler_(static_cast<const EventType&>(event));
    }
    
    std::type_index get_event_type() const override {
        return std::type_index(typeid(EventType));
    }

private:
    HandlerFunc handler_;
};

class EventBus {
public:
    static EventBus& instance() {
        static EventBus bus;
        return bus;
    }

    template<typename EventType>
    void subscribe(std::function<void(const EventType&)> handler) {
        auto typed_handler = std::make_shared<EventHandler<EventType>>(std::move(handler));
        
        std::lock_guard<std::shared_mutex> lock(handlers_mutex_);
        handlers_[std::type_index(typeid(EventType))].push_back(typed_handler);
    }

    template<typename EventType>
    void unsubscribe() {
        std::lock_guard<std::shared_mutex> lock(handlers_mutex_);
        handlers_.erase(std::type_index(typeid(EventType)));
    }

    template<typename EventType>
    void publish(const EventType& event) {
        if (async_mode_.load()) {
            auto event_copy = std::make_unique<EventType>(event);
            
            {
                std::lock_guard<std::mutex> lock(queue_mutex_);
                event_queue_.push(std::move(event_copy));
            }
            queue_cv_.notify_one();
        } else {
            dispatch_event(event);
        }
    }

    template<typename EventType, typename... Args>
    void emit(Args&&... args) {
        EventType event(std::forward<Args>(args)...);
        publish(event);
    }

    void set_async_mode(bool async) {
        async_mode_.store(async);
        if (async && !worker_thread_.joinable()) {
            start_worker_thread();
        }
    }

    void flush() {
        if (!async_mode_.load()) return;
        
        std::unique_lock<std::mutex> lock(queue_mutex_);
        queue_cv_.wait(lock, [this] { return event_queue_.empty(); });
    }

    void start_worker_thread() {
        worker_thread_ = std::thread(&EventBus::worker_loop, this);
    }

    void shutdown() {
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            shutdown_requested_ = true;
        }
        queue_cv_.notify_one();
        
        if (worker_thread_.joinable()) {
            worker_thread_.join();
        }
    }

    ~EventBus() {
        shutdown();
    }

private:
    EventBus() : async_mode_(false), shutdown_requested_(false) {}

    template<typename EventType>
    void dispatch_event(const EventType& event) {
        std::shared_lock<std::shared_mutex> lock(handlers_mutex_);
        
        auto it = handlers_.find(std::type_index(typeid(EventType)));
        if (it != handlers_.end()) {
            for (const auto& handler : it->second) {
                try {
                    handler->handle(event);
                } catch (const std::exception& e) {
                    // Log error but continue processing other handlers
                }
            }
        }
    }

    void worker_loop() {
        while (true) {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            queue_cv_.wait(lock, [this] { 
                return !event_queue_.empty() || shutdown_requested_; 
            });

            if (shutdown_requested_ && event_queue_.empty()) {
                break;
            }

            while (!event_queue_.empty()) {
                auto event = std::move(event_queue_.front());
                event_queue_.pop();
                lock.unlock();
                
                // Dispatch event based on its type
                dispatch_polymorphic_event(*event);
                
                lock.lock();
            }
        }
    }

    void dispatch_polymorphic_event(const Event& event) {
        std::shared_lock<std::shared_mutex> lock(handlers_mutex_);
        
        auto it = handlers_.find(event.type_id);
        if (it != handlers_.end()) {
            for (const auto& handler : it->second) {
                try {
                    handler->handle(event);
                } catch (const std::exception& e) {
                    // Log error but continue processing other handlers
                }
            }
        }
    }

    std::unordered_map<std::type_index, std::vector<std::shared_ptr<IEventHandler>>> handlers_;
    mutable std::shared_mutex handlers_mutex_;
    
    std::queue<std::unique_ptr<Event>> event_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    std::thread worker_thread_;
    
    std::atomic<bool> async_mode_;
    std::atomic<bool> shutdown_requested_;
};

#define DECLARE_EVENT(EventName) \
    class EventName : public hft::core::TypedEvent<EventName>

} // namespace hft::core