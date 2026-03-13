#ifndef SERVER_HANDLERS_INGEST_HANDLER_HH
#define SERVER_HANDLERS_INGEST_HANDLER_HH

#include "../../common/json_utils.hh"
#include "../db/postgres_client.hh"
#include "../pubsub/redis_client.hh"
#include <boost/beast/http.hpp>
#include <string>

namespace http = boost::beast::http;

// Handles POST /ingest — writes sensor readings to Postgres and publishes to Redis.
class IngestHandler {
private:
    // Database client used to persist sensor readings.
    PostgresClient* db_client;

    // Redis client used to publish readings to sensor channels.
    RedisClient* redis_client;

public:
    // Construct with pointers to the shared db and redis clients.
    IngestHandler(PostgresClient* db_client, RedisClient* redis_client);

    // Accept a single JSON object or a JSON array of sensor readings.
    // Returns 201 Created with {"status":"ok","count":N} on success.
    // Template must stay in header — instantiated at each call site.
    template<class Body, class Allocator>
    http::response<http::string_body>
    handle(const http::request<Body, http::basic_fields<Allocator>>& req) {

        http::response<http::string_body> res;
        res.version(req.version());
        res.keep_alive(req.keep_alive());
        res.set(http::field::server, "IoT-Server");
        res.set(http::field::content_type, "application/json");

        if (req.method() != http::verb::post) {
            res.result(http::status::method_not_allowed);
            res.body() = R"({"error":"Method not allowed"})";
            res.prepare_payload();
            return res;
        }

        try {
            std::string body_str = req.body();
            json request_json = json::parse(body_str);

            std::vector<SensorData> readings;
            if (request_json.is_array()) {
                for (auto& elem : request_json)
                    readings.push_back(elem.get<SensorData>());
            } else {
                readings.push_back(request_json.get<SensorData>());
            }

            int ok_count = 0;
            for (const auto& data : readings) {
                bool inserted = db_client->insertEvent(data);
                bool upserted = db_client->upsertLatest(data);
                if (inserted && upserted) {
                    redis_client->publish("sensor:" + data.sensor_id, json(data).dump());
                    std::cout << "Ingested: " << data.sensor_id
                              << " = " << data.value << " " << data.unit << std::endl;
                    ++ok_count;
                }
            }

            if (ok_count == 0) {
                res.result(http::status::internal_server_error);
                res.body() = R"({"error":"Database operation failed"})";
                res.prepare_payload();
                return res;
            }

            res.result(http::status::created);
            res.body() = json{{"status", "ok"}, {"count", ok_count}}.dump();
            res.prepare_payload();
            return res;

        } catch (const json::exception& e) {
            res.result(http::status::bad_request);
            res.body() = json{{"error", "Invalid JSON"}, {"details", e.what()}}.dump();
            res.prepare_payload();
            return res;

        } catch (const std::exception& e) {
            res.result(http::status::internal_server_error);
            res.body() = json{{"error", "Internal server error"}, {"details", e.what()}}.dump();
            res.prepare_payload();
            return res;
        }
    }
};

#endif // SERVER_HANDLERS_INGEST_HANDLER_HH
