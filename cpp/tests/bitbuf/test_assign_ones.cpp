#include "test_common.h"

BITBUF_TEST(assign_ones_inline) {
    BitBuf b{};
    b.assign_ones(17);
    ASSERT_EQ(b.len(), 17u);
    ASSERT_EQ(read_u64(b, 0, 17), mask_u64(17));
}

BITBUF_TEST(assign_ones_heap) {
    BitBuf b{};
    b.assign_ones(256);
    ASSERT_EQ(b.get_bit(0), 1);
    ASSERT_EQ(b.get_bit(255), 1);
}
