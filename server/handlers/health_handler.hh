#ifndef SERVER_HANDLERS_HEALTH_HANDLER_HH
#define SERVER_HANDLERS_HEALTH_HANDLER_HH

#include "../db/postgres_client.hh"
#include "../pubsub/redis_client.hh"
#include "../../common/json_utils.hh"
#include <boost/beast/http.hpp>

namespace http = boost::beast::http;

// Handles GET /health — reports liveness of Postgres and Redis.
class HealthHandler {
private:
    // Database client whose connection pool is health-checked.
    PostgresClient* db_client;

    // Redis client whose connection is health-checked.
    RedisClient* redis_client;

public:
    // Construct with pointers to the shared db and redis clients.
    HealthHandler(PostgresClient* db_client, RedisClient* redis_client);

    // Return 200 {"status":"healthy",...} or 503 {"status":"unhealthy",...}.
    // Template must stay in header — instantiated at each call site.
    template<class Body, class Allocator>
    http::response<http::string_body>
    handle(const http::request<Body, http::basic_fields<Allocator>>& req) {

        http::response<http::string_body> res;
        res.version(req.version());
        res.keep_alive(req.keep_alive());
        res.set(http::field::server, "IoT-Server");
        res.set(http::field::content_type, "application/json");

        if (req.method() != http::verb::get) {
            res.result(http::status::method_not_allowed);
            res.body() = R"({"error":"Method not allowed"})";
            res.prepare_payload();
            return res;
        }

        bool db_healthy    = db_client->healthCheck();
        bool redis_healthy = redis_client->healthCheck();

        json response = {
            {"status", (db_healthy && redis_healthy) ? "healthy" : "unhealthy"},
            {"db",     db_healthy    ? "ok" : "error"},
            {"redis",  redis_healthy ? "ok" : "error"}
        };

        res.result(db_healthy && redis_healthy
                   ? http::status::ok
                   : http::status::service_unavailable);
        res.body() = response.dump();
        res.prepare_payload();
        return res;
    }
};

#endif // SERVER_HANDLERS_HEALTH_HANDLER_HH
