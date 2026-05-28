#include "test_common.h"

BITBUF_TEST(set_ones_range) {
    BitBuf b{};
    b.assign_zeros(32);
    b.set_ones(4, 8);
    ASSERT_EQ(read_u64(b, 4, 8), 0xffull);
}

BITBUF_TEST(set_ones_zero_width_noop) {
    BitBuf b{};
    assign_u64(b, 0x5a, 8);
    b.set_ones(2, 0);
    ASSERT_EQ(read_u64(b, 0, 8), 0x5aull);
}
