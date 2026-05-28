#pragma once

#include "bitbuf.h"
#include <doctest/doctest.h>

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

#define BITBUF_TEST(name) TEST_CASE(#name)
#define ASSERT_TRUE(cond) CHECK((cond))
#define ASSERT_EQ(actual, expected) CHECK((actual) == (expected))

inline uint64_t mask_u64(uint32_t width) {
    if (width == 0) {
        return 0;
    }
    if (width >= 64) {
        return ~0ull;
    }
    return (1ull << width) - 1;
}

inline void assign_u64(BitBuf &buf, uint64_t value, uint32_t width) {
    buf.assign(&value, width);
}

inline uint64_t read_u64(const BitBuf &buf, uint32_t pos, uint32_t width) {
    if (width == 0) {
        return 0;
    }
    BitBuf::data_t out = 0;
    buf.get_bits(pos, width, &out);
    return out;
}
