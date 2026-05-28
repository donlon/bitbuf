#include "test_common.h"

BITBUF_TEST(delete_high_partial_and_full) {
    BitBuf b{};
    assign_u64(b, 0b110101u, 6);
    b.delete_high(2);
    ASSERT_EQ(b.len(), 4u);
    ASSERT_EQ(read_u64(b, 0, 4), 0b0101u);

    b.delete_high(100);
    ASSERT_EQ(b.len(), 0u);
}
