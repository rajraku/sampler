#ifndef SERVER_HANDLERS_INGEST_HANDLER_HH
#define SERVER_HANDLERS_INGEST_HANDLER_HH

#include "../../common/json_utils.hh"
#include "../db/postgres_client.hh"
#include "../pubsub/redis_client.hh"
#include <boost/beast/http.hpp>
#include <string>

namespace http = boost::beast::http;

class IngestHandler {
private:
    PostgresClient* db_client_;
    RedisClient* redis_client_;

public:
    IngestHandler(PostgresClient* db_client, RedisClient* redis_client)
        : db_client_(db_client), redis_client_(redis_client) {}

    // Handle POST /ingest request
    template<class Body, class Allocator>
    http::response<http::string_body> handle(
        const http::request<Body, http::basic_fields<Allocator>>& req) {

        http::response<http::string_body> res;
        res.version(req.version());
        res.keep_alive(req.keep_alive());
        res.set(http::field::server, "IoT-Server");
        res.set(http::field::content_type, "application/json");

        // Only accept POST requests
        if (req.method() != http::verb::post) {
            res.result(http::status::method_not_allowed);
            res.body() = R"({"error":"Method not allowed"})";
            res.prepare_payload();
            return res;
        }

        try {
            // Parse JSON body
            std::string body_str = req.body();
            json request_json = json::parse(body_str);

            // Validate required fields
            if (!request_json.contains("sensor_id") ||
                !request_json.contains("sensor_type") ||
                !request_json.contains("device_name") ||
                !request_json.contains("value") ||
                !request_json.contains("unit") ||
                !request_json.contains("timestamp")) {

                res.result(http::status::bad_request);
                res.body() = R"({"error":"Missing required fields"})";
                res.prepare_payload();
                return res;
            }

            // Convert to SensorData
            SensorData data = request_json.get<SensorData>();

            // Insert into database (both tables)
            bool event_inserted = db_client_->insertEvent(data);
            bool latest_updated = db_client_->upsertLatest(data);

            if (!event_inserted || !latest_updated) {
                res.result(http::status::internal_server_error);
                res.body() = R"({"error":"Database operation failed"})";
                res.prepare_payload();
                return res;
            }

            // Publish to Redis
            std::string channel = "sensor:" + data.sensor_id;
            json message_json = data;
            redis_client_->publish(channel, message_json.dump());

            // Return success response
            res.result(http::status::created);
            json response_json = {
                {"status", "ok"},
                {"sensor_id", data.sensor_id}
            };
            res.body() = response_json.dump();
            res.prepare_payload();

            std::cout << "Ingested sensor data: " << data.sensor_id
                      << " = " << data.value << " " << data.unit << std::endl;

            return res;

        } catch (const json::exception& e) {
            res.result(http::status::bad_request);
            json error_json = {
                {"error", "Invalid JSON"},
                {"details", e.what()}
            };
            res.body() = error_json.dump();
            res.prepare_payload();
            return res;

        } catch (const std::exception& e) {
            res.result(http::status::internal_server_error);
            json error_json = {
                {"error", "Internal server error"},
                {"details", e.what()}
            };
            res.body() = error_json.dump();
            res.prepare_payload();
            return res;
        }
    }
};

#endif // SERVER_HANDLERS_INGEST_HANDLER_HH
