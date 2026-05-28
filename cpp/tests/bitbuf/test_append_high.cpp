#include "test_common.h"

BITBUF_TEST(append_high_increases_length_and_places_bits_high) {
    BitBuf b{};
    assign_u64(b, 0b1011u, 4);
    BitBuf::data_t high = 0b10u;
    b.append_high(&high, 2);
    ASSERT_EQ(b.len(), 6u);
    ASSERT_EQ(read_u64(b, 0, 6), 0b1011u | (0b10u << 4));
}
