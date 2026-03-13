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

class StreamHandler {
private:
    SSEManager* sse_manager_;
    PostgresClient* db_client_;

    // Parse query parameters from URL
    std::set<std::string> parse_sensor_filters(const std::string& target) {
        std::set<std::string> filters;

        // Find the query string
        size_t query_pos = target.find('?');
        if (query_pos == std::string::npos) {
            return filters; // No query parameters, return empty set (all sensors)
        }

        std::string query = target.substr(query_pos + 1);

        // Parse "sensors=sensor1,sensor2,sensor3"
        size_t sensors_pos = query.find("sensors=");
        if (sensors_pos == std::string::npos) {
            return filters; // No sensors parameter, return empty set (all sensors)
        }

        std::string sensors_str = query.substr(sensors_pos + 8); // Skip "sensors="

        // Split by comma
        std::istringstream ss(sensors_str);
        std::string sensor;
        while (std::getline(ss, sensor, ',')) {
            if (!sensor.empty()) {
                filters.insert(sensor);
            }
        }

        return filters;
    }

public:
    StreamHandler(SSEManager* sse_manager, PostgresClient* db_client)
        : sse_manager_(sse_manager), db_client_(db_client) {}

    // Handle GET /stream request
    // This function returns an HTTP response header, but the connection
    // remains open for streaming. The actual streaming is handled by SSEManager.
    template<class Body, class Allocator>
    http::response<http::string_body> handle(
        const http::request<Body, http::basic_fields<Allocator>>& req,
        std::shared_ptr<boost::asio::ip::tcp::socket> socket) {

        http::response<http::string_body> res;
        res.version(req.version());
        res.keep_alive(true); // Keep connection alive for streaming
        res.set(http::field::server, "IoT-Server");
        res.set(http::field::content_type, "text/event-stream");
        res.set(http::field::cache_control, "no-cache");
        res.set(http::field::connection, "keep-alive");
        res.set(http::field::access_control_allow_origin, "*");

        // Only accept GET requests
        if (req.method() != http::verb::get) {
            res.result(http::status::method_not_allowed);
            res.body() = "Method not allowed\n";
            res.prepare_payload();
            return res;
        }

        try {
            // Parse sensor filters from query parameters
            std::string target = std::string(req.target());
            std::set<std::string> sensor_filters = parse_sensor_filters(target);

            std::cout << "SSE stream request with " << sensor_filters.size()
                      << " sensor filters" << std::endl;

            // Register this connection with SSE manager
            size_t conn_id = sse_manager_->register_connection(socket, sensor_filters);

            // Get initial state from database
            std::vector<SensorData> initial_state;
            if (!sensor_filters.empty()) {
                std::vector<std::string> filter_vec(sensor_filters.begin(), sensor_filters.end());
                initial_state = db_client_->getLatest(filter_vec);
            }

            // Send initial state
            sse_manager_->send_initial_state(conn_id, initial_state);

            // Return SSE headers
            // The connection will remain open and SSEManager will handle streaming
            res.result(http::status::ok);
            res.body() = ""; // Empty body, streaming will happen separately
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
