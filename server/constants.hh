#ifndef SERVER_CONSTANTS_HH
#define SERVER_CONSTANTS_HH

#include <cstddef>
#include <string_view>

namespace constants {

// HTTP server identity and content types
inline constexpr std::string_view SERVER_NAME        = "IoT-Server";
inline constexpr std::string_view CONTENT_TYPE_JSON  = "application/json";

// Route paths
inline constexpr std::string_view PATH_INGEST = "/ingest";
inline constexpr std::string_view PATH_HEALTH = "/health";
inline constexpr std::string_view PATH_STREAM = "/stream";

// Stream handler query parameter for sensor filtering
inline constexpr std::string_view QUERY_PARAM_SENSORS = "sensors=";

// Redis URL parsing
inline constexpr std::string_view REDIS_URL_SCHEME    = "tcp://";
inline constexpr std::string_view REDIS_DEFAULT_HOST  = "127.0.0.1";
inline constexpr int              REDIS_DEFAULT_PORT   = 6379;
inline constexpr int              REDIS_SOCKET_TIMEOUT_MS = 100;

// Redis pub/sub channel naming
inline constexpr std::string_view REDIS_CHANNEL_PREFIX    = "sensor:";
inline constexpr std::string_view REDIS_SUBSCRIBE_PATTERN = "sensor:*";

// SSE event names
inline constexpr std::string_view SSE_EVENT_SENSOR_UPDATE = "sensor_update";
inline constexpr std::string_view SSE_EVENT_HEARTBEAT     = "heartbeat";

// Database connection pool
inline constexpr size_t DB_POOL_SIZE_DEFAULT    = 10;
inline constexpr int    DB_MAX_RETRIES          = 3;    // attempts in fresh_connection()
inline constexpr int    DB_EXECUTE_MAX_ATTEMPTS = 2;    // attempts in execute_with_retry()
inline constexpr int    DB_RECONNECT_SLEEP_MS   = 1000;
inline constexpr int    DB_RETRY_SLEEP_MS       = 200;

} // namespace constants

#endif // SERVER_CONSTANTS_HH
