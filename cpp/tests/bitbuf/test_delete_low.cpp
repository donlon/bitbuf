// SPDX-License-Identifier: MIT

#include "test_common.h"

TEST_SUITE_BEGIN("delete_low");

BITBUF_TEST(delete_low_partial_and_full) {
    BitBuf b{};
    assign_u64(b, 0b110101u, 6);
    b.delete_low(2);
    CHECK(b.len() == 4u);
    CHECK(read_u64(b, 0, 4) == 0b1101u);

    b.delete_low(100);
    CHECK(b.len() == 0u);
}

TEST_SUITE_END();
