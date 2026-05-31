#include "test_common.h"

TEST_SUITE_BEGIN("width");

BITBUF_TEST(width_matches_len) {
    BitBuf b{};
    b.assign_ones(77);
    CHECK(b.width() == b.len());
}

TEST_SUITE_END();
