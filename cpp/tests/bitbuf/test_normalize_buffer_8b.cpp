// SPDX-License-Identifier: MIT

#include "test_common.h"

TEST_SUITE_BEGIN("normalize_buffer_8b");

TEST_CASE_FIXTURE(DoubleWordFixture, "normalize_buffer_8b_aligned") {
    const auto SHIFTS = 8;
    b.delete_low(SHIFTS);
    auto *ptr = (uint64_t *) b.normalize_buffer_8b();
    uint64_t expectedWord0 = ((initialWord0 >> SHIFTS)) + ((initialWord1 & ((1ull << SHIFTS) - 1)) << (64 - SHIFTS));
    uint64_t expectedWord1 = ((initialWord1 >> SHIFTS));
    CHECK((uint8_t *) ptr == ((uint8_t *) b.inline_buffer + 8 + 1));
    CHECK(ptr[0] == expectedWord0);
    CHECK((ptr[1] & ((1ull << 56) - 1)) == expectedWord1);
    CHECK((ptr[2] & ((1ull << 56) - 1)) == initialPlaceholder >> SHIFTS);
    // TODO: check write over size?
}

TEST_CASE_FIXTURE(SingleWordFixture, "normalize_buffer_8b_single_word_unaligned") {
    const auto SHIFTS = 12;
    b.delete_low(SHIFTS);
    auto *ptr = (uint64_t *) b.normalize_buffer_8b();
    uint64_t expectedWord0 = (initialWord0 >> SHIFTS);
    CHECK(b.offset == 64 + 8);
    CHECK((ptr[0] & ((1ull << 56) - 1)) == expectedWord0);
}

TEST_CASE_FIXTURE(DoubleWordFixture, "normalize_buffer_8b_unaligned") {
    const auto SHIFTS = 12;
    b.delete_low(SHIFTS);
    auto *ptr = (uint64_t *) b.normalize_buffer_8b();
    uint64_t expectedWord0 = ((initialWord0 >> SHIFTS)) + ((initialWord1 & ((1ull << SHIFTS) - 1)) << (64 - SHIFTS));
    uint64_t expectedWord1 = ((initialWord1 >> SHIFTS));
    CHECK(b.offset == 64 + 8);
    CHECK(b.length == 128 - 12);
    CHECK(ptr[0] == expectedWord0);
    CHECK((ptr[1] & ((1ull << 52) - 1)) == expectedWord1);
    CHECK((ptr[2] & ((1ull << 52) - 1)) == initialPlaceholder >> SHIFTS);
}

BITBUF_TEST(normalize_buffer_aligns_offset_to_byte) {
    BitBuf b{};
    b.assign_ones(24);
    b.delete_low(3);
    CHECK((b.get_offset() % 8) != 0);

    auto before = read_u64(b, 0, b.len());
    uint8_t *ptr = b.normalize_buffer_8b();
    CHECK(ptr != nullptr);
    CHECK(b.get_offset() % 8 == 0u);
    CHECK(read_u64(b, 0, b.len()) == before);
}


TEST_SUITE_END();
