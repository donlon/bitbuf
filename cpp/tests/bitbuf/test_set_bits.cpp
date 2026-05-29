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
    CHECK(b.inline_buffer[0] == initialPlaceholder);
    CHECK(b.inline_buffer[1] == (initialWord0 & ~mask0 | (value << 8) & mask0));
    CHECK(b.inline_buffer[2] == (initialWord1 & ~mask1 | (value >> 56) & mask1));
    CHECK(b.inline_buffer[3] == initialWord2);
}

TEST_CASE_FIXTURE(MultipleWordsFixture, "set_bits_multiple_words_aligned_2") {
    uint64_t values[2] = {0x1234567812345678ull, 0x1234567812345678ull};
    b.set_bits(8, values, 70);
    uint64_t mask0 = 0xffffffffffffff00ull;
    uint64_t mask1 = 0x0000000000003fffull;
    CHECK(b.inline_buffer[0] == initialPlaceholder);
    CHECK(b.inline_buffer[1] == (initialWord0 & ~mask0 | (values[0] << 8) & mask0));
    CHECK(b.inline_buffer[2] == (initialWord1 & ~mask1 | ((values[0] >> 56) + (values[1] << 8)) & mask1));
    CHECK(b.inline_buffer[3] == initialWord2);
}

TEST_CASE_FIXTURE(MultipleWordsFixture, "set_bits_multiple_words_unaligned") {
    uint64_t value = 0xccccccccddddddddull;
    b.set_bits(12, &value, 64);
    uint64_t mask0 = 0xfffffffffffff000ull;
    uint64_t mask1 = 0x0000000000000fffull;
    CHECK(b.inline_buffer[0] == initialPlaceholder);
    CHECK(b.inline_buffer[1] == (initialWord0 & ~mask0 | (value << 12) & mask0));
    CHECK(b.inline_buffer[2] == (initialWord1 & ~mask1 | (value >> 52) & mask1));
    CHECK(b.inline_buffer[3] == initialWord2);
}

TEST_CASE_FIXTURE(MultipleWordsFixture, "set_bits_multiple_words_unaligned_2") {
    uint64_t value = 0xccccccccddddddddull;
    b.set_bits(12, &value, 70);
    uint64_t mask0 = 0xfffffffffffff000ull;
    uint64_t mask1 = 0x000000000003ffffull;
    CHECK(b.inline_buffer[0] == initialPlaceholder);
    CHECK(b.inline_buffer[1] == (initialWord0 & ~mask0 | (value << 12) & mask0));
    CHECK(b.inline_buffer[2] == (initialWord1 & ~mask1 | (value >> 52) & mask1));
    CHECK(b.inline_buffer[3] == initialWord2);
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

TEST_SUITE_END();
