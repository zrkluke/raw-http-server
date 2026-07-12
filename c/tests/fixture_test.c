#include "fixture.h"

#include <assert.h>
#include <string.h>

int main(int argc, char **argv) {
    static const unsigned char expected_input[] = "shared fixture input\n";
    static const unsigned char expected_output[] = "shared fixture expected\n";
    struct fixture fixture;

    assert(argc == 2);
    assert(fixture_load(argv[1], "foundation", "loader-smoke", &fixture) == 0);
    assert(fixture.input_len == sizeof(expected_input) - 1);
    assert(memcmp(fixture.input, expected_input, fixture.input_len) == 0);
    assert(fixture.expected_len == sizeof(expected_output) - 1);
    assert(memcmp(fixture.expected, expected_output, fixture.expected_len) == 0);
    fixture_free(&fixture);
    return 0;
}
