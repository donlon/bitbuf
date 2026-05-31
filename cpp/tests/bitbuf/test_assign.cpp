#include "test_common.h"
#include <cstring>

TEST_SUITE_BEGIN("assign");

BITBUF_TEST(assign_simple) {
    BitBuf b{};
    CHECK(b.offset == BitBuf::initialOffset);
    assign_u64(b, 0x2d, 6);
    CHECK(b.offset == BitBuf::initialOffset);
    CHECK(b.length == 6u);
    CHECK(!b.using_heap_buffer());
    CHECK((b.get_buffer()[BitBuf::initialOffsetWords] & 0x3f) == 0x2d);
    CHECK(b.len() == 6u);

    b.assign(nullptr, 0);
    CHECK(b.len() == 0u);
    CHECK(b.offset == BitBuf::initialOffset);
}

BITBUF_TEST(assign_from) {
    BitBuf b = BitBuf::from<uint32_t>(10, 5);
    CHECK(b.offset == BitBuf::initialOffset);
    CHECK(b.length == 5);
    CHECK(!b.using_heap_buffer());
    CHECK((b.inline_buffer[BitBuf::initialOffsetWords] & 0x1f) == 10);
}

BITBUF_TEST(assign_from_autosize) {
    BitBuf b = BitBuf::from<uint64_t>(0xabcd1234cdea0123);
    CHECK(b.offset == BitBuf::initialOffset);
    CHECK(b.length == 64);
    CHECK(!b.using_heap_buffer());
    CHECK(b.inline_buffer[BitBuf::initialOffsetWords] == 0xabcd1234cdea0123);
}

BITBUF_TEST(assign_heap_boundary) {
    BitBuf b{};
    b.assign_zeros(64);
    CHECK(b.len() == 64);

    BitBuf::data_t words[8] = {
            0x0123456789abcdefull,
            0xfedcba9876543210ull,
            0xaaaaaaaa55555555ull,
            0xcccccccceeeeeeeeull,
            0x1122334455667788ull,
            0xaabbccddeeaabbccull,
            0xddeeffddeeffddeeull,
            0x2233445566778899ull,
    };
    b.assign(words, 8 * 64);
    CHECK(b.len() == 8 * 64);
    CHECK(b.offset == BitBuf::initialHeapOffset);
    CHECK(b.using_heap_buffer());
    CHECK(memcmp(&b.heap_buffer[BitBuf::initialHeapOffsetWords], words, sizeof(words)) == 0);
}

BITBUF_TEST(assign_ones_inline) {
    BitBuf b{};
    b.assign_ones(17);
    CHECK(b.offset == BitBuf::initialOffset);
    CHECK(b.length == 17u);
    CHECK((b.inline_buffer[0] & 0x1ffff) == 0);
}

BITBUF_TEST(assign_ones_heap) {
    BitBuf b{};
    b.assign_ones(64 * 8);
    CHECK(b.length == 64 * 8);
    CHECK(b.offset == BitBuf::initialHeapOffset);
    for (unsigned int i = 0; i < 8; i++) CHECK(b.heap_buffer[BitBuf::initialHeapOffsetWords + i] == -1ull);
}

BITBUF_TEST(assign_zeros_inline) {
    BitBuf b{};
    assign_u64(b, ~0ull, 32);
    b.assign_zeros(32);
    CHECK(b.len() == 32u);
    CHECK(read_u64(b, 0, 32) == 0ull);
}

BITBUF_TEST(assign_zeros_heap) {
    BitBuf b{};
    b.assign_ones(200);
    b.assign_zeros(200);
    CHECK(b.len() == 200u);
    CHECK(b.get_bit(0) == 0);
    CHECK(b.get_bit(199) == 0);
}

TEST_SUITE_END();
