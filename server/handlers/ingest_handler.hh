#ifndef SERVER_HANDLERS_INGEST_HANDLER_HH
#define SERVER_HANDLERS_INGEST_HANDLER_HH

#include "handler_utils.hh"
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

        if (auto err = check_method(req, http::verb::post)) return *err;
        auto res = make_json_response(req);

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
                return make_error(req, http::status::internal_server_error,
                                  "Database operation failed");
            }

            res.result(http::status::created);
            res.body() = json{{"status", "ok"}, {"count", ok_count}}.dump();
            res.prepare_payload();
            return res;

        } catch (const json::exception& e) {
            return make_error(req, http::status::bad_request, "Invalid JSON", e.what());

        } catch (const std::exception& e) {
            return make_error(req, http::status::internal_server_error,
                              "Internal server error", e.what());
        }
    }
};

#endif // SERVER_HANDLERS_INGEST_HANDLER_HH
