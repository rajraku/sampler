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

// Represents a single open SSE connection to a browser client.
class SSEConnection {
public:
    using socket_type = boost::asio::ip::tcp::socket;

    // Sensor IDs to forward; empty means forward all sensors.
    std::set<std::string> sensor_filters;

    // The open TCP socket for this connection.
    std::shared_ptr<socket_type> socket;

    // Unique identifier assigned by SSEManager.
    size_t id;

    // Construct with connection ID, socket, and optional sensor filter set.
    SSEConnection(size_t conn_id, std::shared_ptr<socket_type> sock,
                  const std::set<std::string>& filters);

    // Write an SSE frame to the socket. Returns false if the client has disconnected.
    bool
    send_sse(const std::string& event_name, const std::string& data, size_t event_id);

    // Return true if the socket is still open.
    bool
    is_alive() const;
};

// Manages all open SSE connections and fans out Redis pub/sub messages to them.
class SSEManager {
private:
    // Active connections keyed by their ID.
    std::map<size_t, std::shared_ptr<SSEConnection>> connections;

    // Guards connections and next_connection_id.
    std::mutex connections_mutex;

    // Monotonically increasing ID assigned to the next registered connection.
    size_t next_connection_id;

    // Monotonically increasing ID included in each SSE frame.
    size_t next_event_id;

    // Redis client used to subscribe to sensor channels.
    RedisClient* redis_client;

public:
    // Subscribe to all sensor:* channels on redis_client.
    SSEManager(RedisClient* redis_client);

    // Register a new SSE connection and return its ID.
    size_t
    register_connection(std::shared_ptr<SSEConnection::socket_type> socket,
                        const std::set<std::string>& sensor_filters);

    // Remove the connection with the given ID.
    void
    unregister_connection(size_t conn_id);

    // Look up a connection by ID. Returns nullptr if not found.
    std::shared_ptr<SSEConnection>
    get_connection(size_t conn_id);

    // Push all rows of initial_data to the connection as sensor_update events.
    void
    send_initial_state(size_t conn_id, const std::vector<SensorData>& initial_data);

    // Called by the Redis subscriber; fans the message out to matching connections.
    void
    on_redis_message(const std::string& channel, const std::string& message);

    // Send a heartbeat event to all alive connections.
    void
    send_heartbeat();

    // Remove connections whose sockets have been closed.
    void
    cleanup_dead_connections();
};

#endif // SERVER_SSE_SSE_MANAGER_HH
