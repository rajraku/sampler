#ifndef SERVER_HANDLERS_HEALTH_HANDLER_HH
#define SERVER_HANDLERS_HEALTH_HANDLER_HH

#include "../db/postgres_client.hh"
#include "../pubsub/redis_client.hh"
#include "../../common/json_utils.hh"
#include <boost/beast/http.hpp>

namespace http = boost::beast::http;

class HealthHandler {
private:
    PostgresClient* db_client_;
    RedisClient* redis_client_;

public:
    HealthHandler(PostgresClient* db_client, RedisClient* redis_client)
        : db_client_(db_client), redis_client_(redis_client) {}

    // Handle GET /health request
    template<class Body, class Allocator>
    http::response<http::string_body> handle(
        const http::request<Body, http::basic_fields<Allocator>>& req) {

        http::response<http::string_body> res;
        res.version(req.version());
        res.keep_alive(req.keep_alive());
        res.set(http::field::server, "IoT-Server");
        res.set(http::field::content_type, "application/json");

        // Only accept GET requests
        if (req.method() != http::verb::get) {
            res.result(http::status::method_not_allowed);
            res.body() = R"({"error":"Method not allowed"})";
            res.prepare_payload();
            return res;
        }

        // Check database health
        bool db_healthy = db_client_->healthCheck();

        // Check Redis health
        bool redis_healthy = redis_client_->healthCheck();

        // Build response
        json response = {
            {"status", (db_healthy && redis_healthy) ? "healthy" : "unhealthy"},
            {"db", db_healthy ? "ok" : "error"},
            {"redis", redis_healthy ? "ok" : "error"}
        };

        if (db_healthy && redis_healthy) {
            res.result(http::status::ok);
        } else {
            res.result(http::status::service_unavailable);
        }

        res.body() = response.dump();
        res.prepare_payload();

        return res;
    }
};

#endif // SERVER_HANDLERS_HEALTH_HANDLER_HH
