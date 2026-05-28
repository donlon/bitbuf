#include "test_common.h"

BITBUF_TEST(pop_high_extracts_and_deletes_bits) {
    BitBuf b{};
    assign_u64(b, 0b110101u, 6);
    BitBuf::data_t out = 0;
    b.pop_high(&out, 3);
    ASSERT_EQ(out, 0b110u);
    ASSERT_EQ(b.len(), 3u);
    ASSERT_EQ(read_u64(b, 0, 3), 0b101u);
}

BITBUF_TEST(pop_high_zero_width_noop) {
    BitBuf b{};
    assign_u64(b, 0x35, 6);
    BitBuf::data_t out = 123;
    b.pop_high(&out, 0);
    ASSERT_EQ(out, 123u);
    ASSERT_EQ(b.len(), 6u);
}
