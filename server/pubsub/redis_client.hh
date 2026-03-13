#ifndef SERVER_PUBSUB_REDIS_CLIENT_HH
#define SERVER_PUBSUB_REDIS_CLIENT_HH

#include <sw/redis++/redis++.h>
#include <string>
#include <iostream>
#include <memory>
#include <functional>
#include <thread>
#include <atomic>

using namespace sw::redis;

class RedisClient {
private:
    std::unique_ptr<Redis> redis_;
    std::unique_ptr<Subscriber> subscriber_;
    std::thread subscriber_thread_;
    std::atomic<bool> running_;

public:
    RedisClient(const std::string& url) : running_(false) {
        try {
            ConnectionOptions opts;
            opts.host = "127.0.0.1";
            opts.port = 6379;
            opts.socket_timeout = std::chrono::milliseconds(100);

            redis_ = std::make_unique<Redis>(opts);
            subscriber_ = std::make_unique<Subscriber>(redis_->subscriber());

            std::cout << "Redis client connected to " << opts.host << ":" << opts.port << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Failed to connect to Redis: " << e.what() << std::endl;
            throw;
        }
    }

    ~RedisClient() {
        stop_subscriber();
    }

    // Publish a message to a channel
    bool publish(const std::string& channel, const std::string& message) {
        try {
            redis_->publish(channel, message);
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Failed to publish to Redis: " << e.what() << std::endl;
            return false;
        }
    }

    // Subscribe to a pattern and call callback on messages
    void subscribe_pattern(const std::string& pattern,
                          std::function<void(std::string, std::string)> callback) {
        try {
            subscriber_->on_pmessage([callback](std::string pattern, std::string channel, std::string msg) {
                callback(channel, msg);
            });

            subscriber_->psubscribe(pattern);
            running_ = true;

            // Start subscriber thread
            subscriber_thread_ = std::thread([this]() {
                while (running_) {
                    try {
                        subscriber_->consume();
                    } catch (const TimeoutError& e) {
                        // Timeout is expected, continue
                        continue;
                    } catch (const std::exception& e) {
                        std::cerr << "Subscriber error: " << e.what() << std::endl;
                        break;
                    }
                }
            });

            std::cout << "Subscribed to Redis pattern: " << pattern << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Failed to subscribe to pattern: " << e.what() << std::endl;
            throw;
        }
    }

    // Stop the subscriber thread
    void stop_subscriber() {
        if (running_) {
            running_ = false;
            if (subscriber_thread_.joinable()) {
                subscriber_thread_.join();
            }
        }
    }

    // Health check
    bool healthCheck() {
        try {
            redis_->ping();
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Redis health check failed: " << e.what() << std::endl;
            return false;
        }
    }
};

#endif // SERVER_PUBSUB_REDIS_CLIENT_HH
