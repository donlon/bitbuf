#include "test_common.h"

TEST_SUITE_BEGIN("len");

BITBUF_TEST(len_reports_bit_count) {
    BitBuf b{};
    CHECK(b.len() == 0u);
    b.assign_ones(73);
    CHECK(b.len() == 73u);
}

TEST_SUITE_END();
