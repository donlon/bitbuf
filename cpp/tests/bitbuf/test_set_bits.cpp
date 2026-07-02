// SPDX-License-Identifier: MIT

#include "test_common.h"

TEST_SUITE_BEGIN("set_bits");

TEST_CASE_FIXTURE(SingleWordFixture, "set_bits_single_word") {
    uint64_t value = 0xccccccccddddddddull;
    b.set_bits(0, &value, 64);
    CHECK(b.inline_buffer[0] == initialPlaceholder);
    CHECK(b.inline_buffer[1] == value);
    CHECK(b.inline_buffer[2] == initialPlaceholder);
}

TEST_CASE_FIXTURE(SingleWordFixture, "set_bits_single_word_2") {
    uint64_t value = 0xccccccccddddddddull;
    b.set_bits(0, &value, 48);
    uint64_t mask = 0xffffffffffff;
    CHECK(b.inline_buffer[0] == initialPlaceholder);
    CHECK(b.inline_buffer[1] == (initialWord0 & ~mask | value & mask));
    CHECK(b.inline_buffer[2] == initialPlaceholder);
}

TEST_CASE_FIXTURE(SingleWordFixture, "set_bits_single_word_3") {
    uint64_t value = 0xccccccccddddddddull;
    b.set_bits(12, &value, 48);
    uint64_t mask = 0xffffffffffff000;
    CHECK(b.inline_buffer[0] == initialPlaceholder);
    CHECK(b.inline_buffer[1] == (initialWord0 & ~mask | (value << 12) & mask));
    CHECK(b.inline_buffer[2] == initialPlaceholder);
}

TEST_CASE_FIXTURE(MultipleWordsFixture, "set_bits_multiple_words_aligned") {
    uint64_t value = 0xccccccccddddddddull;
    b.set_bits(8, &value, 64);
    uint64_t mask0 = 0xffffffffffffff00ull;
    uint64_t mask1 = 0x00000000000000ffull;
    CHECK(b.heap_buffer[1] == initialPlaceholder);
    CHECK(b.heap_buffer[2] == (initialWord0 & ~mask0 | (value << 8) & mask0));
    CHECK(b.heap_buffer[3] == (initialWord1 & ~mask1 | (value >> 56) & mask1));
    CHECK(b.heap_buffer[4] == initialWord2);
}

TEST_CASE_FIXTURE(MultipleWordsFixture, "set_bits_multiple_words_aligned_2") {
    uint64_t values[2] = {0x1234567812345678ull, 0x1234567812345678ull};
    b.set_bits(8, values, 70);
    uint64_t mask0 = 0xffffffffffffff00ull;
    uint64_t mask1 = 0x0000000000003fffull;
    CHECK(b.heap_buffer[1] == initialPlaceholder);
    CHECK(b.heap_buffer[2] == (initialWord0 & ~mask0 | (values[0] << 8) & mask0));
    CHECK(b.heap_buffer[3] == (initialWord1 & ~mask1 | ((values[0] >> 56) + (values[1] << 8)) & mask1));
    CHECK(b.heap_buffer[4] == initialWord2);
}

TEST_CASE_FIXTURE(MultipleWordsFixture, "set_bits_multiple_words_unaligned") {
    uint64_t value = 0xccccccccddddddddull;
    b.set_bits(12, &value, 64);
    uint64_t mask0 = 0xfffffffffffff000ull;
    uint64_t mask1 = 0x0000000000000fffull;
    CHECK(b.heap_buffer[1] == initialPlaceholder);
    CHECK(b.heap_buffer[2] == (initialWord0 & ~mask0 | (value << 12) & mask0));
    CHECK(b.heap_buffer[3] == (initialWord1 & ~mask1 | (value >> 52) & mask1));
    CHECK(b.heap_buffer[4] == initialWord2);
}

TEST_CASE_FIXTURE(MultipleWordsFixture, "set_bits_multiple_words_unaligned_2") {
    uint64_t value = 0xccccccccddddddddull;
    b.set_bits(12, &value, 70);
    uint64_t mask0 = 0xfffffffffffff000ull;
    uint64_t mask1 = 0x000000000003ffffull;
    CHECK(b.heap_buffer[1] == initialPlaceholder);
    CHECK(b.heap_buffer[2] == (initialWord0 & ~mask0 | (value << 12) & mask0));
    CHECK(b.heap_buffer[3] == (initialWord1 & ~mask1 | (value >> 52) & mask1));
    CHECK(b.heap_buffer[4] == initialWord2);
}

BITBUF_TEST(set_bits_basic_and_cross_word) {
    BitBuf b{};
    b.assign_zeros(128);

    BitBuf::data_t src[2] = {0xf000000000000000ull, 0xfull};
    b.set_bits(0, src, 68);
    CHECK(read_u64(b, 60, 8) == 0xffull);
}

BITBUF_TEST(set_bits_out_of_range_noop) {
    BitBuf b{};
    b.assign_ones(16);
    BitBuf::data_t src = 0;
    b.set_bits(10, &src, 8);
    CHECK(read_u64(b, 0, 16) == mask_u64(16));
}

// Regression for an out-of-bounds access in set_bits_nocheck: when an unaligned
// write range ends exactly on a 64-bit word boundary (buf_last_offset == 0),
// buf_last points one word past the data. The old code still did a no-op
// read-modify-write of *buf_last, reading/writing out of bounds. Because the
// write rewrote the same bits, the result stayed correct -- so these assert
// correctness AND are meant to be run under a sanitizer (BITBUF_SANITIZE=ON) to
// catch the OOB itself.
BITBUF_TEST(set_bits_unaligned_end_on_word_boundary_tight_heap) {
    // Tight heap allocation (no spare capacity) so the word one past the data is
    // genuinely out of bounds. Under a sanitizer this deterministically catches
    // the buf_last_offset == 0 overflow; the padded fixtures above do not.
    BitBuf b{};
    constexpr uint32_t words = 6;
    b.heap_buffer = new BitBuf::data_t[words](); // owned; ~BitBuf frees it
    b.capacity = words;                          // > inlineBufferWords -> heap storage
    b.offset = 64;                               // object data starts in word 1
    b.length = words * 64 - 64;                  // ends exactly on the word `words` boundary

    BitBuf::data_t src[words];
    for (auto &w : src) w = ~0ull;
    b.set_bits(1, src, b.length - 1); // unaligned start, range ends on the boundary

    CHECK(read_u64(b, 1, 63) == mask_u64(63));
    CHECK(read_u64(b, 0, 1) == 0ull);
}

TEST_SUITE_END();
