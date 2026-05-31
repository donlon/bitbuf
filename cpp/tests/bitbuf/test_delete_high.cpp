#include "test_common.h"

TEST_SUITE_BEGIN("delete_high");

BITBUF_TEST(delete_high_partial_and_full) {
    BitBuf b{};
    assign_u64(b, 0b110101u, 6);
    b.delete_high(2);
    CHECK(b.len() == 4u);
    CHECK(read_u64(b, 0, 4) == 0b0101u);

    b.delete_high(100);
    CHECK(b.len() == 0u);
}

TEST_SUITE_END();
