
CC = g++

CFLAGS = -Wall -Wextra -g -std=c++17 -fPIC -fPIE

LINKFLAGS = -pthread

TARGET = client_app

SOURCES = device.cc main.cc
OBJECTS = $(SOURCES:.cc=.o)


all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(LINKFLAGS) $^


%.o: %.cc
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)