#include "test_common.h"

BITBUF_TEST(get_offset_changes_with_delete_low) {
    BitBuf b{};
    b.assign_ones(20);
    auto base = b.get_offset();
    b.delete_low(3);
    ASSERT_EQ(b.get_offset(), base + 3);
}
