#include "test_common.h"

TEST_SUITE_BEGIN("ensure_empty_buffer");

BITBUF_TEST(ensure_empty_buffer_1) {
    BitBuf b{};
    b.ensure_empty_buffer(32);
    CHECK(!b.using_heap_buffer());
    CHECK(b.offset == 64);
    CHECK(b.length == 32);
}

BITBUF_TEST(ensure_empty_buffer_2) {
    BitBuf b{};
    b.offset = 32;
    b.length = 64;
    b.ensure_empty_buffer((BitBuf::inlineBufferWords - BitBuf::initialOffsetWords) * 64);
    CHECK(!b.using_heap_buffer());
    CHECK(b.offset == 64);
    CHECK(b.length == (BitBuf::inlineBufferWords - BitBuf::initialOffsetWords) * 64);
}

TEST_SUITE_END();
