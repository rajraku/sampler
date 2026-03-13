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
    // Underlying Redis connection used for publishing.
    std::unique_ptr<Redis> redis;

    // Subscriber instance used for pattern subscriptions.
    std::unique_ptr<Subscriber> subscriber;

    // Background thread that calls subscriber->consume() in a loop.
    std::thread subscriber_thread;

    // Set to false to stop the subscriber thread.
    std::atomic<bool> running;

public:
    // Connect to Redis at the given URL (tcp://host:port).
    RedisClient(const std::string& url);

    ~RedisClient();

    // Publish message to channel. Returns true on success.
    bool
    publish(const std::string& channel, const std::string& message);

    // Subscribe to a glob pattern and invoke callback(channel, message) for each match.
    void
    subscribe_pattern(const std::string& pattern,
                      std::function<void(std::string, std::string)> callback);

    // Stop the subscriber thread and join it.
    void
    stop_subscriber();

    // Ping Redis to verify connectivity. Returns true on success.
    bool
    healthCheck();
};

#endif // SERVER_PUBSUB_REDIS_CLIENT_HH
