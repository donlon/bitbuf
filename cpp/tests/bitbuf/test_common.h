// SPDX-License-Identifier: MIT

#pragma once

#include "bitbuf.h"
#include <doctest/doctest.h>

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

#define BITBUF_TEST(name) TEST_CASE(#name)

struct SingleWordFixture {
    BitBuf b{};
    const uint32_t initialOffset = 64;
    const uint32_t initialLength = 64;
    const uint64_t initialWord0 = 0x7c6c5c4c3c2c1c0cull;
    const uint64_t initialPlaceholder = 0x1111111111111111ull;

    SingleWordFixture() {
        for (uint64_t &i : b.inline_buffer) i = initialPlaceholder;
        b.inline_buffer[1] = initialWord0;
        b.offset = initialOffset;
        b.length = initialLength;
    }
};

struct DoubleWordFixture {
    BitBuf b{};
    const uint32_t initialOffset = 64;
    const uint32_t initialLength = 128;
    const uint64_t initialWord0 = 0x7c6c5c4c3c2c1c0cull;
    const uint64_t initialWord1 = 0xfcecdcccbcac9c8cull;
    const uint64_t initialPlaceholder = 0x1111111111111111ull;

    DoubleWordFixture() {
        for (uint64_t &i : b.inline_buffer) i = initialPlaceholder;
        b.inline_buffer[1] = initialWord0;
        b.inline_buffer[2] = initialWord1;
        b.offset = initialOffset;
        b.length = initialLength;
    }
};

struct MultipleWordsFixture {
    BitBuf b{};
    const uint32_t initialOffset = BitBuf::initialHeapOffset;
    const uint32_t initialLength = 64 * 4;
    const uint64_t initialWord0 = 0x7c6c5c4c3c2c1c0cull;
    const uint64_t initialWord1 = 0xfcecdcccbcac9c8cull;
    const uint64_t initialWord2 = 0x7d6d5d4d3d2d1d0dull;
    const uint64_t initialWord3 = 0xfdedddcdbdad9d8dull;
    const uint64_t initialPlaceholder = 0x1111111111111111ull;

    MultipleWordsFixture() {
        uint64_t *buf = b.ensure_empty_buffer(initialLength);
        CHECK(b.using_heap_buffer());
        CHECK(buf == &b.heap_buffer[BitBuf::initialHeapOffsetWords]);
        for (size_t i = 0; i < b.capacity; i++) b.get_buffer()[i] = initialPlaceholder;
        buf[0] = initialWord0;
        buf[1] = initialWord1;
        buf[2] = initialWord2;
        buf[3] = initialWord3;
        CHECK(b.offset == initialOffset);
        CHECK(b.length == initialLength);
    }
};

struct SingleWordUnalignedFixture : public SingleWordFixture {
    const uint32_t initialOffset = SingleWordFixture::initialOffset + 12;
    const uint32_t initialLength = SingleWordFixture::initialLength - 24;

    SingleWordUnalignedFixture() : SingleWordFixture() {
        b.offset = initialOffset;
        b.length = initialLength;
    }
};

struct DoubleWordUnalignedFixture : public DoubleWordFixture {
    const uint32_t initialOffset = DoubleWordFixture::initialOffset + 12;
    const uint32_t initialLength = DoubleWordFixture::initialLength - 24;

    DoubleWordUnalignedFixture() : DoubleWordFixture() {
        b.offset = initialOffset;
        b.length = initialLength;
    }
};

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
    BitBuf::data_t out = -1ull;
    buf.get_bits(pos, width, &out);
    return out;
}