// SPDX-License-Identifier: MIT

#include "test_common.h"

TEST_SUITE_BEGIN("get_offset");

BITBUF_TEST(get_offset_changes_with_delete_low) {
    BitBuf b{};
    b.assign_ones(20);
    auto base = b.get_offset();
    b.delete_low(3);
    CHECK(b.get_offset() == base + 3);
}

TEST_SUITE_END();
