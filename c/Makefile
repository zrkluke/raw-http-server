CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic
TARGET := raw-http-server-c
TEST_TARGET := fixture-test
TEST_SOURCES := tests/fixture_test.c tests/fixture.c

.PHONY: all test clean

all: $(TARGET)

$(TARGET): main.c
	$(CC) $(CFLAGS) main.c -o $(TARGET)

test: $(TARGET) $(TEST_TARGET)
	./$(TARGET)
	./$(TEST_TARGET) ../testdata

$(TEST_TARGET): $(TEST_SOURCES) tests/fixture.h
	$(CC) $(CFLAGS) -Itests $(TEST_SOURCES) -o $(TEST_TARGET)

clean:
	$(RM) $(TARGET) $(TEST_TARGET)
