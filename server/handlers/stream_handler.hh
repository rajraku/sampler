#ifndef SERVER_HANDLERS_STREAM_HANDLER_HH
#define SERVER_HANDLERS_STREAM_HANDLER_HH

#include "handler_utils.hh"
#include "../sse/sse_manager.hh"
#include "../db/postgres_client.hh"
#include <boost/beast/http.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <string>
#include <set>

namespace http = boost::beast::http;

// Handles GET /stream — upgrades the connection to a Server-Sent Events stream.
class StreamHandler {
private:
    // SSE manager that owns open browser connections.
    SSEManager* sse_manager;

    // Database client used to push initial state to new subscribers.
    PostgresClient* db_client;

    // Extract the comma-separated sensor IDs from the ?sensors= query parameter.
    std::set<std::string>
    parse_sensor_filters(const std::string& target);

public:
    // Construct with pointers to the shared SSE manager and db client.
    StreamHandler(SSEManager* sse_manager, PostgresClient* db_client);

    // Write SSE headers, register the socket with SSEManager, and push initial DB state.
    // Template must stay in header — instantiated at each call site.
    template<class Body, class Allocator>
    void
    handle(const http::request<Body, http::basic_fields<Allocator>>& req,
           std::shared_ptr<boost::asio::ip::tcp::socket> socket) {

        std::set<std::string> sensor_filters = parse_sensor_filters(std::string(req.target()));

        std::cout << "SSE stream request with " << sensor_filters.size()
                  << " sensor filters" << std::endl;

        size_t conn_id = sse_manager->register_connection(socket, sensor_filters);

        if (!sensor_filters.empty()) {
            std::vector<std::string> filter_vec(sensor_filters.begin(), sensor_filters.end());
            auto initial_state = db_client->getLatest(filter_vec);
            sse_manager->send_initial_state(conn_id, initial_state);
        }
    }
};

#endif // SERVER_HANDLERS_STREAM_HANDLER_HH
