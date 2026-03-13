#include "health_handler.hh"

HealthHandler::HealthHandler(PostgresClient* db, RedisClient* redis)
    : db_client(db), redis_client(redis)
{
}
