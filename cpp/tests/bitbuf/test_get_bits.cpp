// SPDX-License-Identifier: MIT

#include "test_common.h"

#include <array>

TEST_SUITE_BEGIN("get_bits");

TEST_CASE_FIXTURE(SingleWordFixture, "get_bits_single_word") {
    CHECK(read_u64(b, 0, 64) == initialWord0);
    CHECK(read_u64(b, 4, 60) == initialWord0 >> 4);
    CHECK(read_u64(b, 0, 60) == (initialWord0 & ((1ull << 60) - 1)));
    CHECK(read_u64(b, 0, 64) == initialWord0);
}

TEST_CASE_FIXTURE(DoubleWordUnalignedFixture, "get_bits_single_word_from_unaligned") {
    uint64_t expectedWord = ((initialWord0 >> 12)) + ((initialWord1 & ((1ull << 16) - 1)) << 52);
    CHECK(read_u64(b, 0, 16) == (expectedWord & 0xffff));
    CHECK(read_u64(b, 0, 64) == expectedWord);
    CHECK(read_u64(b, 4, 60) == expectedWord >> 4);
    CHECK(read_u64(b, 0, 60) == (expectedWord & ((1ull << 60) - 1)));
    CHECK(read_u64(b, 32, 32) == expectedWord >> 32);
}

TEST_CASE_FIXTURE(MultipleWordsFixture, "get_multiple_words_byte_aligned") {
    const auto SHIFTS = 8;
    uint64_t expectedWord0 = ((initialWord0 >> SHIFTS)) + ((initialWord1 & ((1ull << SHIFTS) - 1)) << (64 - SHIFTS));
    uint64_t expectedWord1 = ((initialWord1 >> SHIFTS)) + ((initialWord2 & ((1ull << SHIFTS) - 1)) << (64 - SHIFTS));
    uint64_t expectedWord2 = ((initialWord2 >> SHIFTS)) + ((initialWord3 & ((1ull << SHIFTS) - 1)) << (64 - SHIFTS));
    std::array<uint64_t, 8> result = {};
    result.fill(-1ull);
    b.get_bits(8, 64, result.data());
    CHECK(result[0] == expectedWord0);
    CHECK(result[1] == -1ull);

    result.fill(-1ull);
    b.get_bits(8, 128, result.data());
    CHECK(result[0] == expectedWord0);
    CHECK(result[1] == expectedWord1);
    CHECK(result[2] == -1ull);

    result.fill(-1ull);
    b.get_bits(8, 128 + 12, result.data());
    CHECK(result[0] == expectedWord0);
    CHECK(result[1] == expectedWord1);
    CHECK(result[2] == (expectedWord2 & 0xfff));
    CHECK(result[3] == -1ull);
}

TEST_CASE_FIXTURE(MultipleWordsFixture, "get_multiple_words_byte_unaligned") {
    const auto SHIFTS = 12;
    uint64_t expectedWord0 = ((initialWord0 >> SHIFTS)) + ((initialWord1 & ((1ull << SHIFTS) - 1)) << (64 - SHIFTS));
    uint64_t expectedWord1 = ((initialWord1 >> SHIFTS)) + ((initialWord2 & ((1ull << SHIFTS) - 1)) << (64 - SHIFTS));
    uint64_t expectedWord2 = ((initialWord2 >> SHIFTS)) + ((initialWord3 & ((1ull << SHIFTS) - 1)) << (64 - SHIFTS));
    std::array<uint64_t, 8> result = {};
    result.fill(-1ull);
    b.get_bits(12, 64, result.data());
    CHECK(result[0] == expectedWord0);
    CHECK(result[1] == -1ull);

    result.fill(-1ull);
    b.get_bits(12, 128, result.data());
    CHECK(result[0] == expectedWord0);
    CHECK(result[1] == expectedWord1);
    CHECK(result[2] == -1ull);

    result.fill(-1ull);
    b.get_bits(12, 128 + 12, result.data());
    CHECK(result[0] == expectedWord0);
    CHECK(result[1] == expectedWord1);
    CHECK(result[2] == (expectedWord2 & 0xfff));
    CHECK(result[3] == -1ull);
}

TEST_SUITE_END();
