#ifndef SERVER_HANDLERS_STREAM_HANDLER_HH
#define SERVER_HANDLERS_STREAM_HANDLER_HH

#include "../sse/sse_manager.hh"
#include "../db/postgres_client.hh"
#include <boost/beast/http.hpp>
#include <boost/url.hpp>
#include <string>
#include <sstream>
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
    http::response<http::string_body>
    handle(const http::request<Body, http::basic_fields<Allocator>>& req,
           std::shared_ptr<boost::asio::ip::tcp::socket> socket) {

        http::response<http::string_body> res;
        res.version(req.version());
        res.keep_alive(true);
        res.set(http::field::server, "IoT-Server");
        res.set(http::field::content_type, "text/event-stream");
        res.set(http::field::cache_control, "no-cache");
        res.set(http::field::connection, "keep-alive");
        res.set(http::field::access_control_allow_origin, "*");

        if (req.method() != http::verb::get) {
            res.result(http::status::method_not_allowed);
            res.body() = "Method not allowed\n";
            res.prepare_payload();
            return res;
        }

        try {
            std::string target = std::string(req.target());
            std::set<std::string> sensor_filters = parse_sensor_filters(target);

            std::cout << "SSE stream request with " << sensor_filters.size()
                      << " sensor filters" << std::endl;

            size_t conn_id = sse_manager->register_connection(socket, sensor_filters);

            std::vector<SensorData> initial_state;
            if (!sensor_filters.empty()) {
                std::vector<std::string> filter_vec(sensor_filters.begin(), sensor_filters.end());
                initial_state = db_client->getLatest(filter_vec);
            }

            sse_manager->send_initial_state(conn_id, initial_state);

            res.result(http::status::ok);
            res.body() = "";
            res.prepare_payload();
            return res;

        } catch (const std::exception& e) {
            res.result(http::status::internal_server_error);
            res.body() = std::string("Internal server error: ") + e.what() + "\n";
            res.prepare_payload();
            return res;
        }
    }
};

#endif // SERVER_HANDLERS_STREAM_HANDLER_HH
