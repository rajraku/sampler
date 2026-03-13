#include "postgres_client.hh"

PostgresClient::PostgresClient(const std::string& conn_string, size_t pool_sz)
    : connection_string(conn_string), pool_size(pool_sz), shutdown(false)
{
    for (size_t i = 0; i < pool_size; ++i) {
        try {
            auto conn = std::make_unique<pqxx::connection>(connection_string);
            pool.push(std::move(conn));
        } catch (const std::exception& e) {
            std::cerr << "Failed to create database connection: " << e.what() << std::endl;
            throw;
        }
    }
    std::cout << "PostgreSQL connection pool initialized with " << pool_size << " connections" << std::endl;
}

PostgresClient::~PostgresClient()
{
    shutdown = true;
    pool_cv.notify_all();
}

std::unique_ptr<pqxx::connection>
PostgresClient::fresh_connection()
{
    for (int i = 0; i < 3; ++i) {
        try {
            return std::make_unique<pqxx::connection>(connection_string);
        } catch (const std::exception& e) {
            std::cerr << "Reconnect attempt " << (i + 1) << " failed: " << e.what() << std::endl;
            if (i < 2) std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    return nullptr;
}

std::unique_ptr<pqxx::connection>
PostgresClient::acquire_connection()
{
    std::unique_lock<std::mutex> lock(pool_mutex);
    pool_cv.wait(lock, [this] { return !pool.empty() || shutdown; });

    if (shutdown) return nullptr;

    auto conn = std::move(pool.front());
    pool.pop();
    return conn;
}

void
PostgresClient::release_connection(std::unique_ptr<pqxx::connection> conn)
{
    std::lock_guard<std::mutex> lock(pool_mutex);
    pool.push(std::move(conn));
    pool_cv.notify_one();
}

bool
PostgresClient::insertEvent(const SensorData& data)
{
    return execute_with_retry([&](pqxx::connection& conn) {
        pqxx::work txn(conn);
        txn.exec_params(
            "INSERT INTO sensor_events (sensor_id, sensor_type, device_name, value, unit, timestamp) "
            "VALUES ($1, $2, $3, $4, $5, $6)",
            data.sensor_id, data.sensor_type, data.device_name,
            data.value, data.unit, data.timestamp
        );
        txn.commit();
    });
}

bool
PostgresClient::upsertLatest(const SensorData& data)
{
    return execute_with_retry([&](pqxx::connection& conn) {
        pqxx::work txn(conn);
        txn.exec_params(
            "INSERT INTO sensor_latest (sensor_id, sensor_type, device_name, value, unit, timestamp, updated_at) "
            "VALUES ($1, $2, $3, $4, $5, $6, NOW()) "
            "ON CONFLICT (sensor_id) DO UPDATE SET "
            "value = EXCLUDED.value, unit = EXCLUDED.unit, "
            "timestamp = EXCLUDED.timestamp, updated_at = NOW()",
            data.sensor_id, data.sensor_type, data.device_name,
            data.value, data.unit, data.timestamp
        );
        txn.commit();
    });
}

std::vector<SensorData>
PostgresClient::getLatest(const std::vector<std::string>& sensor_ids)
{
    auto conn = acquire_connection();
    std::vector<SensorData> results;

    if (!conn) return results;

    try {
        pqxx::work txn(*conn);

        std::string query = "SELECT sensor_id, sensor_type, device_name, value, unit, "
                            "to_char(timestamp, 'YYYY-MM-DD\"T\"HH24:MI:SS\"Z\"') as timestamp "
                            "FROM sensor_latest WHERE sensor_id IN (";
        for (size_t i = 0; i < sensor_ids.size(); ++i) {
            query += "$" + std::to_string(i + 1);
            if (i < sensor_ids.size() - 1) query += ", ";
        }
        query += ")";

        pqxx::params params;
        for (const auto& id : sensor_ids) params.append(id);

        auto res = txn.exec_params(query, params);

        for (const auto& row : res) {
            SensorData data;
            data.sensor_id   = row["sensor_id"].as<std::string>();
            data.sensor_type = row["sensor_type"].as<std::string>();
            data.device_name = row["device_name"].as<std::string>();
            data.value       = row["value"].as<double>();
            data.unit        = row["unit"].as<std::string>();
            data.timestamp   = row["timestamp"].as<std::string>();
            results.push_back(data);
        }

        txn.commit();
        release_connection(std::move(conn));
        return results;
    } catch (const std::exception& e) {
        std::cerr << "Failed to get latest: " << e.what() << std::endl;
        release_connection(std::move(conn));
        return results;
    }
}

bool
PostgresClient::healthCheck()
{
    return execute_with_retry([](pqxx::connection& conn) {
        pqxx::work txn(conn);
        txn.exec("SELECT 1");
        txn.commit();
    });
}
