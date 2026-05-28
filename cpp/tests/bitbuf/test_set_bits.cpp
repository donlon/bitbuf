#include "test_common.h"

BITBUF_TEST(set_bits_basic_and_cross_word) {
    BitBuf b{};
    b.assign_zeros(128);

    BitBuf::data_t src[2] = {0xf000000000000000ull, 0xfull};
    b.set_bits(60, src, 68);
    ASSERT_EQ(read_u64(b, 60, 8), 0xffull);
}

BITBUF_TEST(set_bits_out_of_range_noop) {
    BitBuf b{};
    b.assign_ones(16);
    BitBuf::data_t src = 0;
    b.set_bits(10, &src, 8);
    ASSERT_EQ(read_u64(b, 0, 16), mask_u64(16));
}
