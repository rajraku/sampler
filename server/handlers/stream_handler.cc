#include "stream_handler.hh"
#include "../constants.hh"
#include <sstream>

StreamHandler::StreamHandler(SSEManager* sse_mgr, PostgresClient* db)
    : sse_manager(sse_mgr), db_client(db)
{
}

std::set<std::string>
StreamHandler::parse_sensor_filters(const std::string& target)
{
    std::set<std::string> filters;

    size_t query_pos = target.find('?');
    if (query_pos == std::string::npos) return filters;

    std::string query = target.substr(query_pos + 1);

    size_t sensors_pos = query.find(constants::QUERY_PARAM_SENSORS);
    if (sensors_pos == std::string::npos) return filters;

    std::string sensors_str = query.substr(sensors_pos + constants::QUERY_PARAM_SENSORS.size());

    std::istringstream ss(sensors_str);
    std::string sensor;
    while (std::getline(ss, sensor, ',')) {
        if (!sensor.empty()) filters.insert(sensor);
    }

    return filters;
}
