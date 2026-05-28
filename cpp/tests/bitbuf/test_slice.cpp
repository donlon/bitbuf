#include "test_common.h"

BITBUF_TEST(slice_basic_and_zero_width) {
    BitBuf b{};
    assign_u64(b, 0b1101101u, 7);
    BitBuf s = b.slice(1, 4);
    ASSERT_EQ(s.len(), 4u);
    ASSERT_EQ(read_u64(s, 0, 4), 0b0110u);

    BitBuf z = b.slice(0, 0);
    ASSERT_EQ(z.len(), 0u);
}

BITBUF_TEST(slice_out_of_range_returns_empty) {
    BitBuf b{};
    assign_u64(b, 0x7u, 3);
    BitBuf s = b.slice(2, 4);
    ASSERT_EQ(s.len(), 0u);
}
