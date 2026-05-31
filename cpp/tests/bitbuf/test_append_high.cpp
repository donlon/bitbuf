// SPDX-License-Identifier: MIT

#include "test_common.h"

BITBUF_TEST(append_high_increases_length_and_places_bits_high) {
    BitBuf b = BitBuf::from<uint64_t>(0x1122, 16);
    BitBuf::data_t high = 0xaabbccdd;
    b.append_high(&high, 32);
    CHECK(b.len() == 48);
    CHECK(read_u64(b, 0, 48) == 0xaabbccdd1122);
}
