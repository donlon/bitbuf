#include "test_common.h"

BITBUF_TEST(lshift_zero_and_full_width) {
    BitBuf b{};
    assign_u64(b, 0b101011u, 6);
    b.lshift(0);
    ASSERT_EQ(read_u64(b, 0, 6), 0b101011u);

    b.lshift(6);
    ASSERT_EQ(read_u64(b, 0, 6), 0ull);
    ASSERT_EQ(b.len(), 6u);
}
