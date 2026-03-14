#ifndef SERVER_DB_POSTGRES_CLIENT_HH
#define SERVER_DB_POSTGRES_CLIENT_HH

#include "../../common/types.hh"
#include "../constants.hh"
#include <pqxx/pqxx>
#include <string>
#include <memory>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <iostream>
#include <thread>

class PostgresClient {
private:
    // libpq connection string (host, port, dbname, user, password).
    std::string connection_string;

    // Pool of ready-to-use connections.
    std::queue<std::unique_ptr<pqxx::connection>> pool;

    // Guards pool.
    std::mutex pool_mutex;

    // Notified when a connection is returned to pool or shutdown is set.
    std::condition_variable pool_cv;

    // Maximum number of connections kept in the pool.
    size_t pool_size;

    // Set to true in destructor to wake blocked acquire_connection calls.
    bool shutdown;

    // Try to open a brand-new connection, retrying up to 3 times with 1s gaps.
    std::unique_ptr<pqxx::connection>
    fresh_connection();

    // Block until a connection is available, then remove and return it.
    std::unique_ptr<pqxx::connection>
    acquire_connection();

    // Return a connection to the pool and notify one waiter.
    void
    release_connection(std::unique_ptr<pqxx::connection> conn);

    // Run fn(conn), retrying once with a fresh connection on failure.
    // Template must stay in header — instantiated at each call site.
    template<typename Fn>
    bool
    execute_with_retry(Fn&& fn) {
        for (int attempt = 0; attempt < constants::DB_EXECUTE_MAX_ATTEMPTS; ++attempt) {
            auto conn = acquire_connection();
            if (!conn) return false;
            try {
                fn(*conn);
                release_connection(std::move(conn));
                return true;
            } catch (const std::exception& e) {
                std::cerr << "DB error (attempt " << (attempt + 1) << "): " << e.what() << std::endl;
                auto replacement = fresh_connection();
                if (replacement) release_connection(std::move(replacement));
                if (attempt == 0) std::this_thread::sleep_for(std::chrono::milliseconds(constants::DB_RETRY_SLEEP_MS));
            }
        }
        return false;
    }

public:
    // Open pool_size connections to the database described by connection_string.
    PostgresClient(const std::string& connection_string, size_t pool_size = constants::DB_POOL_SIZE_DEFAULT);

    ~PostgresClient();

    // Append one row to sensor_events. Returns true on success.
    bool
    insertEvent(const SensorData& data);

    // Upsert sensor_latest (ON CONFLICT UPDATE). Returns true on success.
    bool
    upsertLatest(const SensorData& data);

    // Fetch the latest reading for each sensor_id in sensor_ids.
    std::vector<SensorData>
    getLatest(const std::vector<std::string>& sensor_ids);

    // Execute SELECT 1 to verify the connection pool is alive.
    bool
    healthCheck();
};

#endif // SERVER_DB_POSTGRES_CLIENT_HH
