#include "test_common.h"

BITBUF_TEST(delete_low_partial_and_full) {
    BitBuf b{};
    assign_u64(b, 0b110101u, 6);
    b.delete_low(2);
    ASSERT_EQ(b.len(), 4u);
    ASSERT_EQ(read_u64(b, 0, 4), 0b1101u);

    b.delete_low(100);
    ASSERT_EQ(b.len(), 0u);
}
