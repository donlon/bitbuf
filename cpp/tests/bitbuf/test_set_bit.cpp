#include "test_common.h"

BITBUF_TEST(set_bit_in_range_and_out_of_range) {
    BitBuf b{};
    b.assign_zeros(8);
    b.set_bit(3, 1);
    ASSERT_EQ(b.get_bit(3), 1);

    b.set_bit(8, 1);
    ASSERT_EQ(b.get_bit(7), 0);
}
