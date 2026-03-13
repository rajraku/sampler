#ifndef SERVER_CONFIG_HH
#define SERVER_CONFIG_HH

#include <string>
#include <cstdlib>

struct ServerConfig {
    std::string db_url;
    std::string redis_url;
    unsigned short port;

    static ServerConfig load() {
        ServerConfig config;

        // Load from environment variables with fallback defaults
        const char* db_url_env = std::getenv("DB_URL");
        config.db_url = db_url_env ? db_url_env :
                        "postgresql://postgres:postgres@localhost:5432/sampler_iot";

        const char* redis_url_env = std::getenv("REDIS_URL");
        config.redis_url = redis_url_env ? redis_url_env :
                           "tcp://127.0.0.1:6379";

        const char* port_env = std::getenv("PORT");
        config.port = port_env ? static_cast<unsigned short>(std::atoi(port_env)) : 8080;

        return config;
    }
};

#endif // SERVER_CONFIG_HH
