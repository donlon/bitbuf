#include "test_common.h"

BITBUF_TEST(width_matches_len) {
    BitBuf b{};
    b.assign_ones(77);
    ASSERT_EQ(b.width(), b.len());
}
