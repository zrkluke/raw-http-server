CC ?= cc
CFLAGS ?= -O0 -g
STRICT_CFLAGS := -std=c11 -Wall -Wextra -Wpedantic -Werror
SANITIZER_FLAGS := -fsanitize=address,undefined -fno-omit-frame-pointer
TARGET := raw-http-server-c
TEST_TARGET := fixture-test
TEST_SOURCES := tests/fixture_test.c tests/fixture.c
BUILD_DIR := build
SANITIZER_TARGET := $(BUILD_DIR)/raw-http-server-c-sanitize
SANITIZER_TEST_TARGET := $(BUILD_DIR)/fixture-test-sanitize

.PHONY: all test sanitize clean

all: $(TARGET)

$(TARGET): main.c Makefile
	$(CC) $(STRICT_CFLAGS) $(CFLAGS) main.c -o $(TARGET)

test: $(TARGET) $(TEST_TARGET)
	./$(TEST_TARGET) ../testdata

$(TEST_TARGET): $(TEST_SOURCES) tests/fixture.h Makefile
	$(CC) $(STRICT_CFLAGS) $(CFLAGS) -Itests $(TEST_SOURCES) -o $(TEST_TARGET)

sanitize: $(SANITIZER_TARGET) $(SANITIZER_TEST_TARGET)
	ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1 ./$(SANITIZER_TEST_TARGET) ../testdata

$(SANITIZER_TARGET): main.c Makefile | $(BUILD_DIR)
	$(CC) $(STRICT_CFLAGS) $(CFLAGS) $(SANITIZER_FLAGS) main.c -o $@

$(SANITIZER_TEST_TARGET): $(TEST_SOURCES) tests/fixture.h Makefile | $(BUILD_DIR)
	$(CC) $(STRICT_CFLAGS) $(CFLAGS) $(SANITIZER_FLAGS) -Itests $(TEST_SOURCES) -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	$(RM) $(TARGET) $(TEST_TARGET)
	$(RM) -r $(BUILD_DIR)
