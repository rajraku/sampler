#ifndef SERVER_SSE_SSE_MANAGER_HH
#define SERVER_SSE_SSE_MANAGER_HH

#include "../pubsub/redis_client.hh"
#include "../../common/json_utils.hh"
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <map>
#include <set>
#include <mutex>
#include <memory>
#include <sstream>

namespace beast = boost::beast;
namespace http = beast::http;

// SSE connection wrapper
class SSEConnection {
public:
    using socket_type = boost::asio::ip::tcp::socket;

    std::set<std::string> sensor_filters;
    std::shared_ptr<socket_type> socket;
    size_t id;

    SSEConnection(size_t conn_id, std::shared_ptr<socket_type> sock,
                  const std::set<std::string>& filters)
        : sensor_filters(filters), socket(sock), id(conn_id) {}

    // Send SSE message
    void send_sse(const std::string& event_name, const std::string& data, size_t event_id) {
        std::ostringstream oss;
        oss << "event: " << event_name << "\n";
        oss << "data: " << data << "\n";
        oss << "id: " << event_id << "\n\n";

        std::string message = oss.str();

        try {
            boost::system::error_code ec;
            boost::asio::write(*socket, boost::asio::buffer(message), ec);
            if (ec) {
                std::cerr << "Failed to send SSE message: " << ec.message() << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "Exception sending SSE: " << e.what() << std::endl;
        }
    }

    bool is_alive() const {
        return socket && socket->is_open();
    }
};

// SSE Manager - manages all SSE connections and Redis subscriptions
class SSEManager {
private:
    std::map<size_t, std::shared_ptr<SSEConnection>> connections_;
    std::mutex connections_mutex_;
    size_t next_connection_id_;
    size_t next_event_id_;
    RedisClient* redis_client_;

public:
    SSEManager(RedisClient* redis_client)
        : next_connection_id_(1), next_event_id_(1), redis_client_(redis_client) {

        // Subscribe to all sensor channels
        redis_client_->subscribe_pattern("sensor:*",
            [this](const std::string& channel, const std::string& message) {
                this->on_redis_message(channel, message);
            });
    }

    // Register a new SSE connection
    size_t register_connection(std::shared_ptr<SSEConnection::socket_type> socket,
                              const std::set<std::string>& sensor_filters) {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        size_t conn_id = next_connection_id_++;

        auto conn = std::make_shared<SSEConnection>(conn_id, socket, sensor_filters);
        connections_[conn_id] = conn;

        std::cout << "SSE connection registered: " << conn_id
                  << " with " << sensor_filters.size() << " filters" << std::endl;

        return conn_id;
    }

    // Unregister an SSE connection
    void unregister_connection(size_t conn_id) {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        connections_.erase(conn_id);
        std::cout << "SSE connection unregistered: " << conn_id << std::endl;
    }

    // Get a connection by ID
    std::shared_ptr<SSEConnection> get_connection(size_t conn_id) {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        auto it = connections_.find(conn_id);
        if (it != connections_.end()) {
            return it->second;
        }
        return nullptr;
    }

    // Broadcast initial state to a connection
    void send_initial_state(size_t conn_id, const std::vector<SensorData>& initial_data) {
        auto conn = get_connection(conn_id);
        if (!conn || !conn->is_alive()) return;

        for (const auto& data : initial_data) {
            json j = data;
            conn->send_sse("sensor_update", j.dump(), next_event_id_++);
        }
    }

    // Handle Redis pub/sub messages
    void on_redis_message(const std::string& channel, const std::string& message) {
        // Extract sensor_id from channel (format: "sensor:device1_speed")
        std::string sensor_id = channel.substr(7); // Remove "sensor:" prefix

        std::lock_guard<std::mutex> lock(connections_mutex_);

        // Broadcast to connections that are filtering for this sensor
        for (auto& pair : connections_) {
            auto& conn = pair.second;

            if (!conn->is_alive()) continue;

            // Check if this connection is filtering for this sensor
            // If filters is empty, send to all; otherwise check if sensor_id matches
            if (conn->sensor_filters.empty() ||
                conn->sensor_filters.count(sensor_id) > 0) {

                conn->send_sse("sensor_update", message, next_event_id_++);
            }
        }
    }

    // Send heartbeat to all connections
    void send_heartbeat() {
        std::lock_guard<std::mutex> lock(connections_mutex_);

        json heartbeat = {{"timestamp", get_current_timestamp_iso8601()}};
        std::string heartbeat_msg = heartbeat.dump();

        for (auto& pair : connections_) {
            auto& conn = pair.second;
            if (conn->is_alive()) {
                conn->send_sse("heartbeat", heartbeat_msg, next_event_id_++);
            }
        }
    }

    // Clean up dead connections
    void cleanup_dead_connections() {
        std::lock_guard<std::mutex> lock(connections_mutex_);

        for (auto it = connections_.begin(); it != connections_.end();) {
            if (!it->second->is_alive()) {
                std::cout << "Removing dead SSE connection: " << it->first << std::endl;
                it = connections_.erase(it);
            } else {
                ++it;
            }
        }
    }
};

#endif // SERVER_SSE_SSE_MANAGER_HH
