#include "test_common.h"

BITBUF_TEST(compare_equal_and_length_mismatch) {
    BitBuf a;
    BitBuf b{};
    assign_u64(a, 0x155, 9);
    assign_u64(b, 0x155, 9);
    ASSERT_TRUE(a.compare(b));

    b.resize(8);
    ASSERT_TRUE(!a.compare(b));
}

BITBUF_TEST(compare_unaligned_paths) {
    BitBuf a;
    BitBuf b{};
    assign_u64(a, 0x3ff, 10);
    assign_u64(b, 0x3ff, 10);
    a.delete_low(1);
    b.delete_low(1);
    ASSERT_TRUE(a.compare(b));
}
