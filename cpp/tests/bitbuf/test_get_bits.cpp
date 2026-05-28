#include "test_common.h"

BITBUF_TEST(get_bits_single_word_boundary) {
    BitBuf b{};
    assign_u64(b, 0xfedcba9876543210ull, 64);
    ASSERT_EQ(read_u64(b, 0, 64), 0xfedcba9876543210ull);
}

BITBUF_TEST(get_bits_cross_word_unaligned) {
    BitBuf::data_t words[2] = {0xf000000000000000ull, 0x000000000000000full};
    BitBuf b{};
    b.assign(words, 128);
    ASSERT_EQ(read_u64(b, 60, 8), 0xffull);
}
