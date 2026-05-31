#include "test_common.h"

TEST_SUITE_BEGIN("get_bit");

BITBUF_TEST(get_bit_boundary_cases) {
    BitBuf b{};
    assign_u64(b, 0b101101u, 6);
    CHECK(b.get_bit(0) == 1);
    CHECK(b.get_bit(5) == 1);
    CHECK(b.get_bit(6) == 0);
}

TEST_SUITE_END();
