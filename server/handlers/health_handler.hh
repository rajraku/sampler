#ifndef SERVER_HANDLERS_HEALTH_HANDLER_HH
#define SERVER_HANDLERS_HEALTH_HANDLER_HH

#include "handler_utils.hh"
#include "../db/postgres_client.hh"
#include "../pubsub/redis_client.hh"
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

        if (auto err = check_method(req, http::verb::get)) return *err;
        auto res = make_json_response(req);

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
