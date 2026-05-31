#include "test_common.h"

TEST_SUITE_BEGIN("set_bit");

BITBUF_TEST(set_bit_in_range_and_out_of_range) {
    BitBuf b{};
    b.assign_zeros(8);
    b.set_bit(3, 1);
    CHECK(b.get_bit(3) == 1);

    b.set_bit(8, 1);
    CHECK(b.get_bit(7) == 0);
}

BITBUF_TEST(set_bit_long) {
    BitBuf b{};
    b.assign_zeros(256);
    b.set_bit(250, 1);
    CHECK(b.get_bit(250) == 1);

    b.set_bit(250, 0);
    CHECK(b.get_bit(250) == 0);
}

TEST_SUITE_END();
