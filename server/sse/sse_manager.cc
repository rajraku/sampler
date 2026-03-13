#include "sse_manager.hh"

// --- SSEConnection ---

SSEConnection::SSEConnection(size_t conn_id, std::shared_ptr<socket_type> sock,
                             const std::set<std::string>& filters)
    : sensor_filters(filters), socket(sock), id(conn_id)
{
}

bool
SSEConnection::send_sse(const std::string& event_name, const std::string& data, size_t event_id)
{
    std::ostringstream oss;
    oss << "event: " << event_name << "\n";
    oss << "data: " << data << "\n";
    oss << "id: " << event_id << "\n\n";

    std::string message = oss.str();

    boost::system::error_code ec;
    boost::asio::write(*socket, boost::asio::buffer(message), ec);
    if (ec) {
        socket->close();
        return false;
    }
    return true;
}

bool
SSEConnection::is_alive() const
{
    return socket && socket->is_open();
}

// --- SSEManager ---

SSEManager::SSEManager(RedisClient* client)
    : next_connection_id(1), next_event_id(1), redis_client(client)
{
    redis_client->subscribe_pattern("sensor:*",
        [this](const std::string& channel, const std::string& message) {
            this->on_redis_message(channel, message);
        });
}

size_t
SSEManager::register_connection(std::shared_ptr<SSEConnection::socket_type> sock,
                                const std::set<std::string>& sensor_filters)
{
    std::lock_guard<std::mutex> lock(connections_mutex);
    size_t conn_id = next_connection_id++;

    auto conn = std::make_shared<SSEConnection>(conn_id, sock, sensor_filters);
    connections[conn_id] = conn;

    std::cout << "SSE connection registered: " << conn_id
              << " with " << sensor_filters.size() << " filters" << std::endl;

    return conn_id;
}

void
SSEManager::unregister_connection(size_t conn_id)
{
    std::lock_guard<std::mutex> lock(connections_mutex);
    connections.erase(conn_id);
    std::cout << "SSE connection unregistered: " << conn_id << std::endl;
}

std::shared_ptr<SSEConnection>
SSEManager::get_connection(size_t conn_id)
{
    std::lock_guard<std::mutex> lock(connections_mutex);
    auto it = connections.find(conn_id);
    if (it != connections.end()) return it->second;
    return nullptr;
}

void
SSEManager::send_initial_state(size_t conn_id, const std::vector<SensorData>& initial_data)
{
    auto conn = get_connection(conn_id);
    if (!conn || !conn->is_alive()) return;

    for (const auto& data : initial_data) {
        json j = data;
        conn->send_sse("sensor_update", j.dump(), next_event_id++);
    }
}

void
SSEManager::on_redis_message(const std::string& channel, const std::string& message)
{
    std::string sensor_id = channel.substr(7); // Remove "sensor:" prefix

    std::lock_guard<std::mutex> lock(connections_mutex);

    std::vector<size_t> dead;

    for (auto& pair : connections) {
        auto& conn = pair.second;

        if (!conn->is_alive()) {
            dead.push_back(conn->id);
            continue;
        }

        if (conn->sensor_filters.empty() ||
            conn->sensor_filters.count(sensor_id) > 0) {

            bool ok = conn->send_sse("sensor_update", message, next_event_id++);
            if (!ok) dead.push_back(conn->id);
        }
    }

    for (size_t id : dead) {
        connections.erase(id);
        std::cout << "SSE connection removed (client disconnected): " << id << std::endl;
    }
}

void
SSEManager::send_heartbeat()
{
    std::lock_guard<std::mutex> lock(connections_mutex);

    json heartbeat = {{"timestamp", get_current_timestamp_iso8601()}};
    std::string heartbeat_msg = heartbeat.dump();

    for (auto& pair : connections) {
        auto& conn = pair.second;
        if (conn->is_alive())
            conn->send_sse("heartbeat", heartbeat_msg, next_event_id++);
    }
}

void
SSEManager::cleanup_dead_connections()
{
    std::lock_guard<std::mutex> lock(connections_mutex);

    for (auto it = connections.begin(); it != connections.end();) {
        if (!it->second->is_alive()) {
            std::cout << "Removing dead SSE connection: " << it->first << std::endl;
            it = connections.erase(it);
        } else {
            ++it;
        }
    }
}
