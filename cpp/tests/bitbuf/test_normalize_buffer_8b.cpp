#include "test_common.h"

BITBUF_TEST(normalize_buffer_aligns_offset_to_byte) {
    BitBuf b{};
    b.assign_ones(24);
    b.delete_low(3);
    ASSERT_TRUE((b.get_offset() % 8) != 0);

    auto before = read_u64(b, 0, b.len());
    uint8_t *ptr = b.normalize_buffer_8b();
    ASSERT_TRUE(ptr != nullptr);
    ASSERT_EQ(b.get_offset() % 8, 0u);
    ASSERT_EQ(read_u64(b, 0, b.len()), before);
}
