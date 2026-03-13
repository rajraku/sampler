#ifndef COMMON_TYPES_HH
#define COMMON_TYPES_HH

#include <string>

struct SensorData {
    std::string sensor_id;      // Unique sensor identifier: "device1_speed"
    std::string sensor_type;    // Type of sensor: "speed", "weight", "temperature"
    std::string device_name;    // Device/client name: "device1"
    double value;               // Sensor reading value
    std::string unit;           // Unit of measurement: "kmph", "kg", "celsius"
    std::string timestamp;      // ISO 8601 timestamp: "2026-03-12T10:30:00Z"
};

#endif // COMMON_TYPES_HH
