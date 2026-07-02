// SPDX-License-Identifier: MIT

#include "test_common.h"

// append_high() logic:
// if (width > 0) {
//     ensure_buffer(0, width);
//     set_bits_nocheck(old_len, value, width);
// }
//
// This file contains only basic tests for append_high(). See test_ensure_buffer.cpp and test_set_bits.cpp for more
// cases.

BITBUF_TEST(append_high_increases_length_and_places_bits_high) {
    BitBuf b = BitBuf::from<uint64_t>(0x1122, 16);
    BitBuf::data_t high = 0xaabbccdd;
    b.append_high(&high, 32);
    CHECK(b.len() == 48);
    CHECK(read_u64(b, 0, 48) == 0xaabbccdd1122);
}
