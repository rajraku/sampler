#ifndef SERVER_DB_POSTGRES_CLIENT_HH
#define SERVER_DB_POSTGRES_CLIENT_HH

#include "../../common/types.hh"
#include <pqxx/pqxx>
#include <string>
#include <memory>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <iostream>

class PostgresClient {
private:
    std::string connection_string_;
    std::queue<std::unique_ptr<pqxx::connection>> pool_;
    std::mutex pool_mutex_;
    std::condition_variable pool_cv_;
    size_t pool_size_;
    bool shutdown_;

    // Get a connection from the pool
    std::unique_ptr<pqxx::connection> acquire_connection() {
        std::unique_lock<std::mutex> lock(pool_mutex_);
        pool_cv_.wait(lock, [this] { return !pool_.empty() || shutdown_; });

        if (shutdown_) {
            return nullptr;
        }

        auto conn = std::move(pool_.front());
        pool_.pop();
        return conn;
    }

    // Return a connection to the pool
    void release_connection(std::unique_ptr<pqxx::connection> conn) {
        std::lock_guard<std::mutex> lock(pool_mutex_);
        pool_.push(std::move(conn));
        pool_cv_.notify_one();
    }

public:
    PostgresClient(const std::string& connection_string, size_t pool_size = 10)
        : connection_string_(connection_string), pool_size_(pool_size), shutdown_(false) {

        // Initialize connection pool
        for (size_t i = 0; i < pool_size_; ++i) {
            try {
                auto conn = std::make_unique<pqxx::connection>(connection_string_);
                pool_.push(std::move(conn));
            } catch (const std::exception& e) {
                std::cerr << "Failed to create database connection: " << e.what() << std::endl;
                throw;
            }
        }
        std::cout << "PostgreSQL connection pool initialized with " << pool_size_ << " connections" << std::endl;
    }

    ~PostgresClient() {
        shutdown_ = true;
        pool_cv_.notify_all();
    }

    // Insert event into sensor_events table
    bool insertEvent(const SensorData& data) {
        auto conn = acquire_connection();
        if (!conn) return false;

        try {
            pqxx::work txn(*conn);
            txn.exec_params(
                "INSERT INTO sensor_events (sensor_id, sensor_type, device_name, value, unit, timestamp) "
                "VALUES ($1, $2, $3, $4, $5, $6)",
                data.sensor_id,
                data.sensor_type,
                data.device_name,
                data.value,
                data.unit,
                data.timestamp
            );
            txn.commit();
            release_connection(std::move(conn));
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Failed to insert event: " << e.what() << std::endl;
            release_connection(std::move(conn));
            return false;
        }
    }

    // Upsert into sensor_latest table (ON CONFLICT UPDATE)
    bool upsertLatest(const SensorData& data) {
        auto conn = acquire_connection();
        if (!conn) return false;

        try {
            pqxx::work txn(*conn);
            txn.exec_params(
                "INSERT INTO sensor_latest (sensor_id, sensor_type, device_name, value, unit, timestamp, updated_at) "
                "VALUES ($1, $2, $3, $4, $5, $6, NOW()) "
                "ON CONFLICT (sensor_id) DO UPDATE SET "
                "value = EXCLUDED.value, "
                "unit = EXCLUDED.unit, "
                "timestamp = EXCLUDED.timestamp, "
                "updated_at = NOW()",
                data.sensor_id,
                data.sensor_type,
                data.device_name,
                data.value,
                data.unit,
                data.timestamp
            );
            txn.commit();
            release_connection(std::move(conn));
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Failed to upsert latest: " << e.what() << std::endl;
            release_connection(std::move(conn));
            return false;
        }
    }

    // Get latest sensor data for given sensor IDs
    std::vector<SensorData> getLatest(const std::vector<std::string>& sensor_ids) {
        auto conn = acquire_connection();
        std::vector<SensorData> results;

        if (!conn) return results;

        try {
            pqxx::work txn(*conn);

            // Build IN clause
            std::string query = "SELECT sensor_id, sensor_type, device_name, value, unit, "
                              "to_char(timestamp, 'YYYY-MM-DD\"T\"HH24:MI:SS\"Z\"') as timestamp "
                              "FROM sensor_latest WHERE sensor_id IN (";

            for (size_t i = 0; i < sensor_ids.size(); ++i) {
                query += "$" + std::to_string(i + 1);
                if (i < sensor_ids.size() - 1) query += ", ";
            }
            query += ")";

            pqxx::params params;
            for (const auto& id : sensor_ids) {
                params.append(id);
            }

            auto res = txn.exec_params(query, params);

            for (const auto& row : res) {
                SensorData data;
                data.sensor_id = row["sensor_id"].as<std::string>();
                data.sensor_type = row["sensor_type"].as<std::string>();
                data.device_name = row["device_name"].as<std::string>();
                data.value = row["value"].as<double>();
                data.unit = row["unit"].as<std::string>();
                data.timestamp = row["timestamp"].as<std::string>();
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

    // Health check
    bool healthCheck() {
        auto conn = acquire_connection();
        if (!conn) return false;

        try {
            pqxx::work txn(*conn);
            txn.exec("SELECT 1");
            txn.commit();
            release_connection(std::move(conn));
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Health check failed: " << e.what() << std::endl;
            release_connection(std::move(conn));
            return false;
        }
    }
};

#endif // SERVER_DB_POSTGRES_CLIENT_HH
