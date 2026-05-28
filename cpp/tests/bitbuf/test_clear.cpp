#include "test_common.h"

BITBUF_TEST(clear_zeroes_and_preserves_length) {
    BitBuf b{};
    b.assign_ones(95);
    b.clear();
    ASSERT_EQ(b.len(), 95u);
    ASSERT_EQ(b.get_bit(0), 0);
    ASSERT_EQ(b.get_bit(94), 0);
}
