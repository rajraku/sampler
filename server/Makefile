
CC = g++

CFLAGS = -Wall -Wextra -g -std=c++26 -fPIC -fPIE

LINKFLAGS = -pthread

TARGET = server_app

SOURCES = main.cc
OBJECTS = $(SOURCES:.cc=.o)


all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(LINKFLAGS) $^


%.o: %.cc
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)