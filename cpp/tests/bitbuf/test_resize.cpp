#include "test_common.h"

BITBUF_TEST(resize_grow_and_shrink) {
    BitBuf b{};
    assign_u64(b, 0x2a, 6);
    b.resize(40);
    ASSERT_EQ(b.len(), 40u);
    ASSERT_EQ(read_u64(b, 0, 6), 0x2aull);

    b.resize(4);
    ASSERT_EQ(b.len(), 4u);
    ASSERT_EQ(read_u64(b, 0, 4), 0xau);
}

BITBUF_TEST(resize_crosses_inline_heap_boundary) {
    BitBuf b{};
    b.resize(200);
    ASSERT_EQ(b.len(), 200u);
    b.resize(16);
    ASSERT_EQ(b.len(), 16u);
}
