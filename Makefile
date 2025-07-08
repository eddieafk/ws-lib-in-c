CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pthread -D_GNU_SOURCE
LDFLAGS = -lssl -lcrypto -lz -lpthread

SRCDIR = src
SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(SOURCES:.c=.o)
TARGET = websocket_server

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

test: $(TARGET)
	./$(TARGET) 8080

install-deps:
	# Ubuntu/Debian
	sudo apt-get update
	sudo apt-get install libssl-dev zlib1g-dev
