// SPDX-License-Identifier: MIT

#include "test_common.h"

TEST_SUITE_BEGIN("toggle");

BITBUF_TEST(toggle_basic) {
    BitBuf b{};
    b.assign_zeros(16);
    b.toggle(4, 4);
    CHECK(read_u64(b, 4, 4) == 0xfull);
    b.toggle(4, 4);
    CHECK(read_u64(b, 4, 4) == 0ull);
}

BITBUF_TEST(toggle_unaligned_span) {
    BitBuf b{};
    b.assign_zeros(80);
    b.toggle(7, 9);
    CHECK(read_u64(b, 7, 9) == 0x1ffull);
}

TEST_SUITE_END();
