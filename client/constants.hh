#ifndef CLIENT_CONSTANTS_HH
#define CLIENT_CONSTANTS_HH

namespace constants {

// HTTP content types
inline constexpr const char* CONTENT_TYPE_JSON = "application/json";

// Server connection defaults — overridden at runtime by the SERVER_URL env var.
inline constexpr const char* SERVER_DEFAULT_HOST    = "localhost";
inline constexpr int         SERVER_DEFAULT_PORT    = 8080;
inline constexpr const char* SERVER_INGEST_PATH     = "/ingest";
inline constexpr int         CONNECTION_TIMEOUT_SEC = 20;

// Milliseconds the sender thread sleeps between consecutive batch POSTs.
inline constexpr int SENDER_SLEEP_MS = 100;

// Default sensor polling interval (seconds).
inline constexpr int DEFAULT_SENSOR_FREQ = 1;

// Initial sensor values written to the Device on construction.
inline constexpr int SPEED_INITIAL  = 26;
inline constexpr int WEIGHT_INITIAL = 6000;
inline constexpr int TEMP_INITIAL   = 24;

// Speed sensor simulation range and metadata.
inline constexpr int         SPEED_MIN  = 25;
inline constexpr int         SPEED_MAX  = 45;
inline constexpr const char* SPEED_TYPE = "speed";
inline constexpr const char* SPEED_UNIT = "kmph";

// Weight sensor simulation range and metadata.
inline constexpr int         WEIGHT_MIN  = 6000;
inline constexpr int         WEIGHT_MAX  = 6005;
inline constexpr const char* WEIGHT_TYPE = "weight";
inline constexpr const char* WEIGHT_UNIT = "kg";

// Temperature sensor simulation range and metadata.
inline constexpr int         TEMP_MIN  = 20;
inline constexpr int         TEMP_MAX  = 30;
inline constexpr const char* TEMP_TYPE = "temperature";
inline constexpr const char* TEMP_UNIT = "celsius";

} // namespace constants

#endif // CLIENT_CONSTANTS_HH
