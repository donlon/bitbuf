#include "test_common.h"

TEST_SUITE_BEGIN("resize");

BITBUF_TEST(resize_grow_and_shrink) {
    BitBuf b{};
    assign_u64(b, 0x2a, 6);
    b.resize(40);
    CHECK(b.len() == 40u);
    CHECK(read_u64(b, 0, 6) == 0x2aull);

    b.resize(4);
    CHECK(b.len() == 4u);
    CHECK(read_u64(b, 0, 4) == 0xau);
}

BITBUF_TEST(resize_crosses_inline_heap_boundary) {
    BitBuf b{};
    b.resize(200);
    CHECK(b.len() == 200u);
    b.resize(16);
    CHECK(b.len() == 16u);
}

TEST_SUITE_END();
