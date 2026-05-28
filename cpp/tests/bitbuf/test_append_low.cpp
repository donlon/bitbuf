#include "test_common.h"

BITBUF_TEST(append_low_increases_length_and_places_bits_low) {
    BitBuf b{};
    assign_u64(b, 0b1011u, 4);
    BitBuf::data_t low = 0b10u;
    b.append_low(&low, 2);
    ASSERT_EQ(b.len(), 6u);
    ASSERT_EQ(read_u64(b, 0, 6), (0b1011u << 2) | 0b10u);
}
