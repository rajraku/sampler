#ifndef COMMON_JSON_UTILS_HH
#define COMMON_JSON_UTILS_HH

#include "../deps/json.hpp"
#include "types.hh"
#include <chrono>
#include <iomanip>
#include <sstream>

using json = nlohmann::json;

// Convert SensorData to JSON
inline void to_json(json& j, const SensorData& data) {
    j = json{
        {"sensor_id", data.sensor_id},
        {"sensor_type", data.sensor_type},
        {"device_name", data.device_name},
        {"value", data.value},
        {"unit", data.unit},
        {"timestamp", data.timestamp}
    };
}

// Convert JSON to SensorData
inline void from_json(const json& j, SensorData& data) {
    j.at("sensor_id").get_to(data.sensor_id);
    j.at("sensor_type").get_to(data.sensor_type);
    j.at("device_name").get_to(data.device_name);
    j.at("value").get_to(data.value);
    j.at("unit").get_to(data.unit);
    j.at("timestamp").get_to(data.timestamp);
}

// Get current timestamp in ISO 8601 format
inline std::string get_current_timestamp_iso8601() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::ostringstream oss;
    oss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count() << 'Z';
    return oss.str();
}

#endif // COMMON_JSON_UTILS_HH
