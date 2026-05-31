// SPDX-License-Identifier: MIT

#include "test_common.h"

TEST_SUITE_BEGIN("slice");

TEST_CASE_FIXTURE(MultipleWordsFixture, "slice_empty") {
    auto empty = b.slice(0, 0);
    CHECK(empty.len() == 0);
}

TEST_CASE_FIXTURE(MultipleWordsFixture, "slice_single_word_aligned") {
    auto empty = b.slice(0, 64);
    CHECK(empty.len() == 64);
    CHECK(empty.inline_buffer[1] == initialWord0);
    // TODO: check out of boundary words
}

TEST_CASE_FIXTURE(MultipleWordsFixture, "slice_single_word_unaligned") {
    auto empty = b.slice(0, 48);
    CHECK(empty.offset == 64);
    CHECK(empty.length == 48);
    CHECK(empty.inline_buffer[1] == initialWord0);
}

TEST_CASE_FIXTURE(MultipleWordsFixture, "slice_single_word_unaligned_2") {
    auto empty = b.slice(48, 16);
    CHECK(empty.offset == 64 + 48);
    CHECK(empty.length == 16);
    CHECK(empty.inline_buffer[1] == initialWord0);
}

TEST_CASE_FIXTURE(MultipleWordsFixture, "slice_single_word_unaligned_3") {
    auto empty = b.slice(40, 20);
    CHECK(empty.offset == 64 + 40);
    CHECK(empty.length == 20);
    CHECK(empty.inline_buffer[1] == initialWord0);
}

TEST_CASE_FIXTURE(MultipleWordsFixture, "slice_multiple_words_aligned") {
    auto empty = b.slice(64, 128);
    CHECK(empty.offset == 64);
    CHECK(empty.length == 128);
    CHECK(empty.inline_buffer[1] == initialWord1);
    CHECK(empty.inline_buffer[2] == initialWord2);
}

TEST_CASE_FIXTURE(MultipleWordsFixture, "slice_multiple_words_unaligned") {
    auto empty = b.slice(12, 128);
    CHECK(empty.offset == 64 + 12);
    CHECK(empty.length == 128);
    CHECK(empty.inline_buffer[1] == initialWord0);
    CHECK(empty.inline_buffer[2] == initialWord1);
    CHECK(empty.inline_buffer[3] == initialWord2);
}

TEST_CASE_FIXTURE(MultipleWordsFixture, "slice_multiple_words_unaligned_2") {
    auto empty = b.slice(12, 128 - 12);
    CHECK(empty.offset == 64 + 12);
    CHECK(empty.length == 128 - 12);
    CHECK(empty.inline_buffer[1] == initialWord0);
    CHECK(empty.inline_buffer[2] == initialWord1);
}

TEST_CASE_FIXTURE(MultipleWordsFixture, "slice_multiple_words_unaligned_3") {
    auto empty = b.slice(0, 128 + 12);
    CHECK(empty.offset == 64);
    CHECK(empty.length == 128 + 12);
    CHECK(empty.inline_buffer[1] == initialWord0);
    CHECK(empty.inline_buffer[2] == initialWord1);
    CHECK(empty.inline_buffer[3] == initialWord2);
}

BITBUF_TEST(slice_out_of_range_returns_empty) {
    BitBuf b{};
    assign_u64(b, 0x7u, 3);
    BitBuf s = b.slice(2, 4);
    CHECK(s.len() == 0u);
}

TEST_SUITE_END();
