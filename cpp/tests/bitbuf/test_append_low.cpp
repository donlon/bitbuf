// SPDX-License-Identifier: MIT

#include "test_common.h"

TEST_SUITE_BEGIN("append_low");

BITBUF_TEST(append_high_increases_length_and_places_bits_low) {
    BitBuf b = BitBuf::from<uint64_t>(0x1122, 16);
    BitBuf::data_t value = 0xaabbccdd;
    b.append_low(&value, 32);
    CHECK(b.len() == 48);
    CHECK(read_u64(b, 0, 48) == 0x1122aabbccdd);
}

TEST_SUITE_END();
