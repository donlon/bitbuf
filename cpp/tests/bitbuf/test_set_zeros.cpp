#include "test_common.h"

BITBUF_TEST(set_zeros_range) {
    BitBuf b{};
    b.assign_ones(32);
    b.set_zeros(5, 10);
    ASSERT_EQ(read_u64(b, 5, 10), 0ull);
}

BITBUF_TEST(set_zeros_zero_width_noop) {
    BitBuf b{};
    assign_u64(b, 0xa5, 8);
    b.set_zeros(3, 0);
    ASSERT_EQ(read_u64(b, 0, 8), 0xa5ull);
}
