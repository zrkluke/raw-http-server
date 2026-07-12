CC ?= cc
CFLAGS ?= -O0 -g
STRICT_CFLAGS := -std=c11 -Wall -Wextra -Wpedantic -Werror
SANITIZER_FLAGS := -fsanitize=address,undefined -fno-omit-frame-pointer
TARGET := raw-http-server-c
TEST_TARGET := fixture-test
LINE_READER_ACCEPTANCE_TARGET := line-reader-acceptance-test
LINE_READER_TEST_TARGET := line-reader-test
TCP_SERVER_ACCEPTANCE_TARGET := tcp-server-acceptance-test
TCP_SERVER_TEST_TARGET := tcp-server-test
TEST_SOURCES := tests/fixture_test.c tests/fixture.c
LINE_READER_SOURCES := line_reader.c
LINE_READER_TEST_SOURCES := tests/line_reader_test.c $(LINE_READER_SOURCES)
LINE_READER_ACCEPTANCE_SOURCES := tests/line_reader_acceptance_test.c tests/fixture.c $(LINE_READER_SOURCES)
BUILD_DIR := build
SANITIZER_TARGET := $(BUILD_DIR)/raw-http-server-c-sanitize
SANITIZER_TEST_TARGET := $(BUILD_DIR)/fixture-test-sanitize
SANITIZER_LINE_READER_ACCEPTANCE_TARGET := $(BUILD_DIR)/line-reader-acceptance-test-sanitize
SANITIZER_LINE_READER_TEST_TARGET := $(BUILD_DIR)/line-reader-test-sanitize
SANITIZER_TCP_SERVER_ACCEPTANCE_TARGET := $(BUILD_DIR)/tcp-server-acceptance-test-sanitize
SANITIZER_TCP_SERVER_TEST_TARGET := $(BUILD_DIR)/tcp-server-test-sanitize
TCP_SERVER_SOURCES := tcp_server.c

.PHONY: all test sanitize clean

all: $(TARGET)

$(TARGET): main.c Makefile
	$(CC) $(STRICT_CFLAGS) $(CFLAGS) main.c -o $(TARGET)

test: $(TARGET) $(TEST_TARGET) $(LINE_READER_ACCEPTANCE_TARGET) $(LINE_READER_TEST_TARGET) $(TCP_SERVER_ACCEPTANCE_TARGET) $(TCP_SERVER_TEST_TARGET)
	./$(TEST_TARGET) ../testdata
	./$(LINE_READER_ACCEPTANCE_TARGET) ../testdata
	./$(LINE_READER_TEST_TARGET)
	./$(TCP_SERVER_ACCEPTANCE_TARGET) ../testdata
	./$(TCP_SERVER_TEST_TARGET)

$(TEST_TARGET): $(TEST_SOURCES) tests/fixture.h Makefile
	$(CC) $(STRICT_CFLAGS) $(CFLAGS) -Itests $(TEST_SOURCES) -o $(TEST_TARGET)

$(LINE_READER_ACCEPTANCE_TARGET): $(LINE_READER_ACCEPTANCE_SOURCES) tests/fixture.h line_reader.h Makefile
	$(CC) $(STRICT_CFLAGS) $(CFLAGS) -I. -Itests $(LINE_READER_ACCEPTANCE_SOURCES) -o $(LINE_READER_ACCEPTANCE_TARGET)

$(LINE_READER_TEST_TARGET): $(LINE_READER_TEST_SOURCES) line_reader.h Makefile
	$(CC) $(STRICT_CFLAGS) $(CFLAGS) -I. $(LINE_READER_TEST_SOURCES) -o $(LINE_READER_TEST_TARGET)

$(TCP_SERVER_ACCEPTANCE_TARGET): tests/tcp_server_acceptance_test.c tests/fixture.c tests/fixture.h tcp_server.h $(TCP_SERVER_SOURCES) Makefile
	$(CC) $(STRICT_CFLAGS) $(CFLAGS) -I. -Itests tests/tcp_server_acceptance_test.c tests/fixture.c $(TCP_SERVER_SOURCES) -o $(TCP_SERVER_ACCEPTANCE_TARGET)

$(TCP_SERVER_TEST_TARGET): tests/tcp_server_test.c tcp_server.h $(TCP_SERVER_SOURCES) Makefile
	$(CC) $(STRICT_CFLAGS) $(CFLAGS) -I. tests/tcp_server_test.c $(TCP_SERVER_SOURCES) -o $(TCP_SERVER_TEST_TARGET)

sanitize: $(SANITIZER_TARGET) $(SANITIZER_TEST_TARGET) $(SANITIZER_LINE_READER_ACCEPTANCE_TARGET) $(SANITIZER_LINE_READER_TEST_TARGET) $(SANITIZER_TCP_SERVER_ACCEPTANCE_TARGET) $(SANITIZER_TCP_SERVER_TEST_TARGET)
	ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1 ./$(SANITIZER_TEST_TARGET) ../testdata
	ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1 ./$(SANITIZER_LINE_READER_ACCEPTANCE_TARGET) ../testdata
	ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1 ./$(SANITIZER_LINE_READER_TEST_TARGET)
	ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1 ./$(SANITIZER_TCP_SERVER_ACCEPTANCE_TARGET) ../testdata
	ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1 ./$(SANITIZER_TCP_SERVER_TEST_TARGET)

$(SANITIZER_TARGET): main.c Makefile | $(BUILD_DIR)
	$(CC) $(STRICT_CFLAGS) $(CFLAGS) $(SANITIZER_FLAGS) main.c -o $@

$(SANITIZER_TEST_TARGET): $(TEST_SOURCES) tests/fixture.h Makefile | $(BUILD_DIR)
	$(CC) $(STRICT_CFLAGS) $(CFLAGS) $(SANITIZER_FLAGS) -Itests $(TEST_SOURCES) -o $@

$(SANITIZER_LINE_READER_ACCEPTANCE_TARGET): $(LINE_READER_ACCEPTANCE_SOURCES) tests/fixture.h line_reader.h Makefile | $(BUILD_DIR)
	$(CC) $(STRICT_CFLAGS) $(CFLAGS) $(SANITIZER_FLAGS) -I. -Itests $(LINE_READER_ACCEPTANCE_SOURCES) -o $@

$(SANITIZER_LINE_READER_TEST_TARGET): $(LINE_READER_TEST_SOURCES) line_reader.h Makefile | $(BUILD_DIR)
	$(CC) $(STRICT_CFLAGS) $(CFLAGS) $(SANITIZER_FLAGS) -I. $(LINE_READER_TEST_SOURCES) -o $@

$(SANITIZER_TCP_SERVER_ACCEPTANCE_TARGET): tests/tcp_server_acceptance_test.c tests/fixture.c tests/fixture.h tcp_server.h $(TCP_SERVER_SOURCES) Makefile | $(BUILD_DIR)
	$(CC) $(STRICT_CFLAGS) $(CFLAGS) $(SANITIZER_FLAGS) -I. -Itests tests/tcp_server_acceptance_test.c tests/fixture.c $(TCP_SERVER_SOURCES) -o $@

$(SANITIZER_TCP_SERVER_TEST_TARGET): tests/tcp_server_test.c tcp_server.h $(TCP_SERVER_SOURCES) Makefile | $(BUILD_DIR)
	$(CC) $(STRICT_CFLAGS) $(CFLAGS) $(SANITIZER_FLAGS) -I. tests/tcp_server_test.c $(TCP_SERVER_SOURCES) -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	$(RM) $(TARGET) $(TEST_TARGET) $(LINE_READER_ACCEPTANCE_TARGET) $(LINE_READER_TEST_TARGET) $(TCP_SERVER_ACCEPTANCE_TARGET) $(TCP_SERVER_TEST_TARGET)
	$(RM) -r $(BUILD_DIR)
