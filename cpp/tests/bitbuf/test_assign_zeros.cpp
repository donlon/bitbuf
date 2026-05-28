#include "test_common.h"

BITBUF_TEST(assign_zeros_inline) {
    BitBuf b{};
    assign_u64(b, ~0ull, 32);
    b.assign_zeros(32);
    ASSERT_EQ(b.len(), 32u);
    ASSERT_EQ(read_u64(b, 0, 32), 0ull);
}

BITBUF_TEST(assign_zeros_heap) {
    BitBuf b{};
    b.assign_ones(200);
    b.assign_zeros(200);
    ASSERT_EQ(b.len(), 200u);
    ASSERT_EQ(b.get_bit(0), 0);
    ASSERT_EQ(b.get_bit(199), 0);
}
