
CC = g++

CFLAGS = -Wall -Wextra -g -std=c++20 -fPIC -fPIE -I/usr/local/include

LINKFLAGS = -pthread -L/usr/local/lib -lpqxx -lpq -lhiredis -lredis++

TARGET = server_app

SOURCES = main.cc \
          db/postgres_client.cc \
          pubsub/redis_client.cc \
          sse/sse_manager.cc \
          handlers/ingest_handler.cc \
          handlers/stream_handler.cc \
          handlers/health_handler.cc
OBJECTS = $(SOURCES:.cc=.o)


all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LINKFLAGS)


%.o: %.cc
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)