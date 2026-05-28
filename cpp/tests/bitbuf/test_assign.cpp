#include "test_common.h"

BITBUF_TEST(assign_inline_and_zero_width) {
    BitBuf b{};
    assign_u64(b, 0x2d, 6);
    ASSERT_EQ(b.len(), 6u);
    ASSERT_EQ(read_u64(b, 0, 6), 0x2dull);

    b.assign(nullptr, 0);
    ASSERT_EQ(b.len(), 0u);
}

BITBUF_TEST(assign_heap_boundary) {
    BitBuf::data_t words[4] = {0x0123456789abcdefull, 0xfedcba9876543210ull, 0xaaaaaaaa55555555ull, 0ull};
    BitBuf b{};
    b.assign(words, 192);
    ASSERT_EQ(b.len(), 192u);
    ASSERT_EQ(b.get_bit(0), 1);
    ASSERT_EQ(b.get_bit(191), 1);
}
