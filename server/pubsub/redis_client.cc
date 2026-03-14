#include "redis_client.hh"
#include "../constants.hh"

RedisClient::RedisClient(const std::string& url) : running(false)
{
    try {
        std::string host = std::string(constants::REDIS_DEFAULT_HOST);
        int port = constants::REDIS_DEFAULT_PORT;
        auto prefix = url.find(constants::REDIS_URL_SCHEME);
        if (prefix != std::string::npos) {
            std::string addr = url.substr(prefix + constants::REDIS_URL_SCHEME.size());
            auto colon = addr.rfind(':');
            if (colon != std::string::npos) {
                host = addr.substr(0, colon);
                port = std::stoi(addr.substr(colon + 1));
            }
        }

        ConnectionOptions opts;
        opts.host = host;
        opts.port = port;
        opts.socket_timeout = std::chrono::milliseconds(constants::REDIS_SOCKET_TIMEOUT_MS);

        redis = std::make_unique<Redis>(opts);
        subscriber = std::make_unique<Subscriber>(redis->subscriber());

        std::cout << "Redis client connected to " << opts.host << ":" << opts.port << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Failed to connect to Redis: " << e.what() << std::endl;
        throw;
    }
}

RedisClient::~RedisClient()
{
    stop_subscriber();
}

bool
RedisClient::publish(const std::string& channel, const std::string& message)
{
    try {
        redis->publish(channel, message);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to publish to Redis: " << e.what() << std::endl;
        return false;
    }
}

void
RedisClient::subscribe_pattern(const std::string& pattern,
                               std::function<void(std::string, std::string)> callback)
{
    try {
        subscriber->on_pmessage([callback](std::string /*pattern*/, std::string channel, std::string msg) {
            callback(channel, msg);
        });

        subscriber->psubscribe(pattern);
        running = true;

        subscriber_thread = std::thread([this]() {
            while (running) {
                try {
                    subscriber->consume();
                } catch (const TimeoutError&) {
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

void
RedisClient::stop_subscriber()
{
    if (running) {
        running = false;
        if (subscriber_thread.joinable())
            subscriber_thread.join();
    }
}

bool
RedisClient::healthCheck()
{
    try {
        redis->ping();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Redis health check failed: " << e.what() << std::endl;
        return false;
    }
}
