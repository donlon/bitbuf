#include "test_common.h"

TEST_SUITE_BEGIN("lshift");

BITBUF_TEST(lshift_zero_and_full_width) {
    BitBuf b{};
    assign_u64(b, 0b101011u, 6);
    b.lshift(0);
    CHECK(read_u64(b, 0, 6) == 0b101011u);


    b.lshift(6);
    CHECK(read_u64(b, 0, 6) == 0ull);
    CHECK(b.len() == 6u);
}

TEST_SUITE_END();
