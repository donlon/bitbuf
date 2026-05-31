// SPDX-License-Identifier: MIT

#include "test_common.h"

TEST_SUITE_BEGIN("compare");

BITBUF_TEST(compare_equal_and_length_mismatch) {
    BitBuf a;
    BitBuf b{};
    assign_u64(a, 0x155, 9);
    assign_u64(b, 0x155, 9);
    CHECK(a.compare(b));

    b.resize(8);
    CHECK(!a.compare(b));
}

BITBUF_TEST(compare_unaligned_paths) {
    BitBuf a;
    BitBuf b{};
    assign_u64(a, 0x3ff, 10);
    assign_u64(b, 0x3ff, 10);
    a.delete_low(1);
    b.delete_low(1);
    CHECK(a.compare(b));
}

TEST_SUITE_END();
