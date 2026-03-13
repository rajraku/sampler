#include "ingest_handler.hh"

IngestHandler::IngestHandler(PostgresClient* db, RedisClient* redis)
    : db_client(db), redis_client(redis)
{
}
