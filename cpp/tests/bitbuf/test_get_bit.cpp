#include "test_common.h"

BITBUF_TEST(get_bit_boundary_cases) {
    BitBuf b{};
    assign_u64(b, 0b101101u, 6);
    ASSERT_EQ(b.get_bit(0), 1);
    ASSERT_EQ(b.get_bit(5), 1);
    ASSERT_EQ(b.get_bit(6), 0);
}
