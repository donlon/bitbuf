#include "test_common.h"

BITBUF_TEST(len_reports_bit_count) {
    BitBuf b{};
    ASSERT_EQ(b.len(), 0u);
    b.assign_ones(73);
    ASSERT_EQ(b.len(), 73u);
}
