# Makefile for host-native unit tests
# Build: make
# Run:   make test

CC      = gcc
CFLAGS  = -Wall -Wextra -std=c99 -g -O0 -lm
TARGET  = test_aqi

.PHONY: all test clean

all: $(TARGET)

$(TARGET): test_aqi.c
	$(CC) $(CFLAGS) -o $@ $<

test: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
