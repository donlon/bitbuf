#include "bitbuf.h"

#include <algorithm>
#include <array>

/// @brief Constructor of the BitBuf object.
BitBuf::BitBuf() : length(0), offset(initialOffset) {}

/// @brief Constructor of the BitBuf object. Initialize data with data in `buffer` and bit count `width`. Data placed in
///        buffer should be held in LSB first format.
BitBuf::BitBuf(const void *buffer, uint32_t width) { assign(buffer, width); }

/// @brief Destructor of the BitBuf object.
BitBuf::~BitBuf() {
    if (using_heap_buffer()) delete[] heap_buffer;
}

/// @brief Assign Bitbuf object with data and size.
/// @param buffer Buffer that contains bits to assign
/// @param width Size of bits to assign
/// @return The bitbuf object
BitBuf &BitBuf::assign(const void *buffer, uint32_t width) {
    ensure_empty_buffer(width);
    if (offset % 8 != 0) throw std::exception();
    memcpy((uint8_t *) get_buffer() + offset / 8, buffer, (width + 7) / 8);
    return *this;
}

/// @brief Assign Bitbuf object with zeros of `width`-bits.
/// @param width Bit size of zeros to assign
/// @return The bitbuf object
BitBuf &BitBuf::assign_zeros(uint32_t width) {
    ensure_empty_buffer(width);
    if (width == 0) return;
    const auto buf = get_buffer();
    const auto buf_first = buf + ((offset) / 64);
    const auto buf_last = buf + ((offset + length - 1) / 64);
    for (auto *ptr = buf_first; ptr <= buf_last; ptr++) { *ptr = 0; }
    // memset(buf_start, 0, (buf_end - buf_start) * sizeof(data_t));
    return *this;
}


/// @brief Assign Bitbuf object with ones of `width`-bits.
/// @param width Bit size of ones to assign
/// @return The bitbuf object
BitBuf &BitBuf::assign_ones(uint32_t width) {
    ensure_empty_buffer(width);
    if (width == 0) return;
    const auto buf = get_buffer();
    const auto buf_first = buf + ((offset) / 64);
    const auto buf_last = buf + ((offset + length - 1) / 64);
    for (auto *ptr = buf_first; ptr <= buf_last; ptr++) { *ptr = -1ull; }
    return *this;
}

/// @brief Compere two BitBuf object. Return true if size and each bit of these two object matches.
/// @param other The other BitBuf object to compare.
/// @return True if these two object matches. False otherwise.
bool BitBuf::compare(BitBuf &other) {
    if (length != other.length) return false;
    uint8_t *ptr1 = normalize_buffer_8b();
    uint8_t *ptr2 = other.normalize_buffer_8b();
    // tailing bits are cleared in normalize_buffer_8b
    return memcmp(ptr1, ptr2, (length + 7) / 8) == 0;
}

/// @brief Get bit size of the object.
/// @return Bit size of the object
uint32_t BitBuf::len() const { return length; }

BitBuf &BitBuf::resize(uint32_t width) {
    ensure_buffer(0, (int64_t) width - (int64_t) length);
    return *this;
}

/// @brief Clear every bit of the object to zero, with size unchanged.
/// @return The bitbuf object
BitBuf &BitBuf::clear() {
    assign_zeros(length);
    return *this;
}

/// @brief Get single bit at @param pos from the object.
/// @param pos Position of the bit to get.
/// @return Bit at position @param pos
int BitBuf::get_bit(uint32_t pos) const {
    if (pos >= length) return 0; // throw nb::index_error("bit range out of range");
    const auto buf = get_buffer();
    const auto buf_start = buf + ((offset + pos) / 64);
    const auto offset_start = ((offset + pos) % 64);
    return (int) ((*buf_start >> offset_start) & 1u);
}

/// @brief Get multiple bits starting from @param pos with size @param width from the object. The result bits are placed
/// in @param dst_buf.
///        Caller of this function should ensure @param dst_buf can hold at lease `(width + 63) / 64` uint64_t words.
/// @param pos Starting position of the bits to get
/// @param width Size of bits to get
/// @param dst_buf Destination buffer to hold result bits
void BitBuf::get_bits(uint32_t pos, uint32_t width, data_t *dst_buf) const {
    if (pos + width > length) throw std::exception();
    get_bits_nocheck(pos, width, dst_buf);
}

void BitBuf::get_bits_nocheck(uint32_t pos, uint32_t width, data_t *dst_buf) const {
    if (width == 0) return;
    const auto buf = get_buffer();
    const auto buf_first = buf + ((offset + pos) / 64);
    const auto buf_last = buf + ((offset + pos + width - 1) / 64);

    const auto offset_start = ((offset + pos) % 64);
    const auto offset_start_c = (64 - offset_start) % 64;
    auto value = *buf_first >> offset_start;
    if (buf_first == buf_last) {
        // C++ says 1ull << width is UB for width == 64 and make them happy
        if (width == 64) {
            *dst_buf = value;
        } else {
            *dst_buf = value & ((1ull << width) - 1);
        }
    } else {
        for (const data_t *ptr = buf_first + 1; ptr <= buf_last; ptr++) {
            *dst_buf++ = value + (*ptr << offset_start_c);
            value = *ptr >> offset_start;
            // TODO: if last, value & ((1ull << (width % 64)) - 1)
        }
    }
}

/// @brief Get multiple bits starting from @param pos with size @param width from the object as a `BitBuf` object.
/// @param pos Starting position of the bits to get
/// @param width Size of bits to get
/// @return A `BitBuf` object containing the result bits.
BitBuf BitBuf::slice(uint32_t pos, uint32_t width) const {
    if (width == 0) return {};
    if (pos + width > length) return {}; // throw nb::index_error("bit range out of range");
    const auto buf_offset = offset + pos;
    const auto buf_length = pos + width;

    BitBuf buf_slice{};
    buf_slice.ensure_empty_buffer(((buf_length + 63) / 64 - (offset + buf_offset) / 64) * 64);
    buf_slice.offset += buf_offset % 64;

    const auto pos_start = (offset + pos) / 64;
    const auto pos_last = (offset + pos + width - 1) / 64;
    const auto data_size = pos_last - pos_start + 1;

    const auto src_start = get_buffer() + pos_start;
    const auto dst_start = buf_slice.get_buffer() + buf_slice.offset / 64;
    memcpy(dst_start, src_start, data_size * sizeof(data_t));
    return buf_slice;
}

/// @brief Set single bit at @param pos to @param value in the object.
/// @param pos Position of the bit to set
/// @param value Value of the bit to set. None zero value are intrepreted as one, otherwize zero.
/// @return The bitbuf object
BitBuf &BitBuf::set_bit(uint32_t pos, uint32_t value) {
    if (pos >= length) return *this; // throw nb::index_error("bit range out of range");
    const auto buf_pos = get_buffer() + (offset + pos) / 64;
    const auto pos_offset = (offset + pos) % 64;
    if (value) {
        *buf_pos |= 1ull << pos_offset;
    } else {
        *buf_pos &= ~(1ull << pos_offset);
    }
    return *this;
}

/// @brief Set multiple bits starting from @param pos with size @param width in the object. Values of the bits are
/// provided from to @param src_buffer. The @param src_buffer should contains at lease `(width + 63) / 64` uint64_t
/// words to read.
/// @param pos Starting position of the bits to set
/// @param width Size of bits to set
/// @param src_buf Source buffer that hold bits to set
/// @return The bitbuf object
BitBuf &BitBuf::set_bits(uint32_t pos, const data_t *src_buffer, uint32_t width) {
    if (pos + width > length) return *this; // nb::index_error("bit range out of range");
    set_bits_nocheck(pos, src_buffer, width);
    return *this;
}

void BitBuf::set_bits_nocheck(uint32_t pos, const data_t *src_buffer, uint32_t width) {
    // TODO: 64b/8b aligned fast path
    const auto buf = get_buffer();
    const auto buf_start = buf + (offset + pos) / 64;
    const auto buf_start_offset = (offset + pos) % 64;
    const auto buf_start_offset_c = (64 - buf_start_offset) % 64;
    const auto buf_last = buf + (offset + pos + width) / 64;
    const auto buf_last_offset = (offset + pos + width) % 64;

    if (buf_start == buf_last) {
        const auto value = *buf_last & (-1ull - (1ull << buf_last_offset) - (1ull << buf_start_offset));
        *buf_last = value | (*src_buffer << buf_start_offset);
    } else {
        auto low_val = (*buf_start & ((1ull << buf_start_offset) - 1));
        for (auto *ptr = buf_start; ptr <= buf_last - 1; ptr++) {
            auto src_value = *src_buffer++;
            *ptr = low_val + (src_value << buf_start_offset);
            low_val = src_value >> buf_start_offset_c;
        }
        auto last_mask = (1ull << buf_last_offset) - 1;
        *buf_last = *buf_last & ~last_mask | low_val & last_mask;
    }
}

template<std::size_t... Is>
constexpr std::array<uint64_t, sizeof...(Is)> make_ones(std::index_sequence<Is...>, uint64_t value) {
    return {{(Is, value)...}};
}

constexpr static const auto ones_buffer = make_ones(std::make_index_sequence<256>{}, -1ull);
constexpr static const auto zeros_buffer = make_ones(std::make_index_sequence<256>{}, 0ull);

/// @brief Set multiple bits starting from @param pos with size @param width in the object to all ones.
/// @param pos Starting position of the bits to set
/// @param width Size of bits to set
/// @return The bitbuf object
BitBuf &BitBuf::set_ones(uint32_t pos, uint32_t width) {
    if (pos + width > length) return *this; // nb::index_error("bit range out of range");
    if (width > ones_buffer.size() * 64) throw std::exception();
    set_bits_nocheck(pos, ones_buffer.data(), width);
    return *this;
}

/// @brief Set multiple bits starting from @param pos with size @param width in the object to all zeros.
/// @param pos Starting position of the bits to set
/// @param width Size of bits to set
/// @return The bitbuf object
BitBuf &BitBuf::set_zeros(uint32_t pos, uint32_t width) {
    // TODO: 64b/8b aligned fast path
    if (pos + width > length) return *this; // nb::index_error("bit range out of range");
    if (width > zeros_buffer.size() * 64) throw std::exception();
    set_bits_nocheck(pos, zeros_buffer.data(), width);
    return *this;
}

/// @brief Toggle multiple bits starting from @param pos with size @param width in the object.
/// @param pos Starting position of the bits to toggle
/// @param width Size of bits to toggle
/// @return The bitbuf object
BitBuf &BitBuf::toggle(uint32_t pos, uint32_t width) {
    if (pos + width > length) return *this; // nb::index_error("bit range out of range");
    uint64_t toggle_buf[16];
    if (width > sizeof(toggle_buf) * 8) throw std::exception();
    get_bits_nocheck(pos, width, toggle_buf);
    size_t p = (width - 1) / 64;
    do { toggle_buf[p] ^= -1ull; } while (p--);
    set_bits_nocheck(pos, toggle_buf, width);
    return *this;
}

/// @brief Shift object to left at @param pos positions, with size of the object unchanged. Bits shift in are filled
/// with zeros.
/// @param bits How many positions to shift left
/// @return The bitbuf object
BitBuf &BitBuf::lshift(uint32_t bits) {
    if (bits == 0) { return *this; }
    if (bits >= length) {
        // Clear to zeros
        assign_zeros(length); // but cap fits
    } else {
        ensure_buffer(bits, 0);
    }
    return *this;
}

/// @brief Shift object to right at @param pos positions, with size of the object unchanged. Bits shift in are filled
/// with zeros.
/// @param bits How many positions to shift right
/// @return The bitbuf object
BitBuf &BitBuf::rshift(uint32_t bits) {
    if (bits == 0) { return *this; }
    if (bits >= length) {
        // Clear to zeros
        assign_zeros(length); // but cap fits
    } else {
        ensure_buffer(-bits, 0);
    }
    return *this;
}

/// @brief Append bits of size @param width at lowest position. Values of the bits are provided from to @param value.
/// The @param value should contains at lease `(width + 63) / 64` uint64_t words to read.
/// @param value Value of the bits to append
/// @param width Size of the bits to append
/// @return The bitbuf object
BitBuf &BitBuf::append_low(const data_t *value, uint32_t width) {
    if (width == 0) { return *this; }
    ensure_buffer(-width, width);
    set_bits_nocheck(0, value, width);
    return *this;
}

/// @brief Append bits of size @param width at highest position. Values of the bits are provided from to @param value.
/// The @param value should contains at lease `(width + 63) / 64` uint64_t words to read.
/// @param value Value of the bits to append
/// @param width Size of the bits to append
/// @return The bitbuf object
BitBuf &BitBuf::append_high(const data_t *value, uint32_t width) {
    if (width == 0) { return *this; }
    const auto old_len = length;
    ensure_buffer(0, width);
    set_bits_nocheck(old_len, value, width);
    return *this;
}

/// @brief Delete bits of size @param width at lowest position.
/// @param width Size of the bits to delete
/// @return The bitbuf object
BitBuf &BitBuf::delete_low(uint32_t width) {
    if (width >= length) {
        length = 0;
        offset = initialOffset; // or heap offset
    } else if (width > 0) {
        length -= width;
        offset += width;
    }
    return *this;
}

/// @brief Delete bits of size @param width at highest position.
/// @param width Size of the bits to delete
/// @return The bitbuf object
BitBuf &BitBuf::delete_high(uint32_t width) {
    if (width >= length) {
        length = 0;
        offset = initialOffset; // or heap offset
    } else if (width > 0) {
        length -= width;
    }
    return *this;
}

/// @brief Get value of bits of size @param width at lowest position and then delete these bits from the object.
/// @param dst_buffer Buffer to hold bits deleted from the object
/// @param width Size of the bits to delete
/// @return
void BitBuf::pop_low(data_t *dst_buffer, uint32_t width) {
    if (width > length) return; // nb::index_error("bit range out of range");
    if (width == 0) return;
    get_bits_nocheck(0, width, dst_buffer);
    length -= width;
    offset += width;
}

/// @brief Get value of bits of size @param width at highest position and then delete these bits from the object.
/// @param dst_buffer Buffer to hold bits deleted from the object
/// @param width Size of the bits to delete
/// @return
void BitBuf::pop_high(data_t *dst_buffer, uint32_t width) {
    if (width > length) return; // nb::index_error("bit range out of range");
    if (width == 0) { return; }
    get_bits_nocheck(length - width, width, dst_buffer);
    length -= width;
}

/// @brief Get bit size of the object.
/// @return Bit size of the object
uint32_t BitBuf::width() const { return length; }

/// @brief Get least byte size that is sufficient to hold bits in the object.
/// @return Least byte size that is sufficient to hold bits in the object
uint32_t BitBuf::nbytes() const { return (length + 7) / 8; }

uint32_t BitBuf::get_offset() const { return offset; }

/// @brief Align offset to 8b and clear bits over `offset + length` position to zeros. Tailing bits in the last uint64_t word is cleared.
/// @return Pointer to aligned offset position at data buffer.
uint8_t *BitBuf::normalize_buffer_8b() {
    const auto buf = get_buffer();
    if (offset % 8 != 0) {
        // TODO: maybe align to initialOffset
        // similar to get_bits_nocheck
        const auto buf_start = buf + (offset / 64);
        const auto buf_last = buf + ((offset + length - 1) / 64);
        const auto shift_offset = offset % 8;
        const auto shift_offset_c = (8 - shift_offset) % 8;
        uint32_t new_offset = offset - shift_offset;

        if (buf_start == buf_last) {
            *buf_start >>= shift_offset;
        } else {
            auto value = *buf_start >> shift_offset;
            for (data_t *ptr = buf_start + 1; ptr <= buf_last; ptr++) {
                *(ptr - 1) = value + (*ptr << shift_offset_c);
                value = *ptr >> shift_offset;
            }
            *buf_last = value & ((1ull << ((new_offset + length) % 64)) - 1);
        }
        offset = new_offset;
    } else {
        const auto buf_last = buf + (offset + length - 1) / 64;
        const auto buf_last_offset = (offset + length) % 64;
        if (buf_last_offset > 0) {
            *buf_last &= (1ull << buf_last_offset) - 1;
        }
    }
    return (uint8_t *) get_buffer() + offset / 8;
}

/// @brief Get if the object is using `heap_buffer` for data. The object is using heap buffer if `capacity` is greater
///        than `inlineBufferWords`, otherwise it's using inline buffer.
/// @return True if the object is using `heap_buffer` for data, false otherwise.
bool BitBuf::using_heap_buffer() const { return capacity > inlineBufferWords; }

const BitBuf::data_t *BitBuf::get_buffer() const { return const_cast<BitBuf *>(this)->get_buffer(); }

BitBuf::data_t *BitBuf::get_buffer() {
    if (using_heap_buffer()) {
        return heap_buffer;
    } else {
        return inline_buffer;
    }
}

/// @brief Ensure buffer size is sufficient for data with size `length`, with word aligned offset.
/// @param new_length New size of data.
void BitBuf::ensure_empty_buffer(uint32_t new_length) {
    const auto minimum_offset = 64;
    if (minimum_offset + new_length <= capacity * 64) {
        this->offset = minimum_offset;
    } else {
        this->offset = heapOffsetWords * 64;
        auto new_capacity = heapOffsetWords + (new_length + 63) / 64 + 1;
        if (using_heap_buffer()) delete[] heap_buffer;
        heap_buffer = new data_t[new_capacity];
        capacity = new_capacity;
    }
    this->length = new_length;
}

/// @brief Ensure buffer size is sufficient for data with growing size `length_delta` and offset `offset_delta`.
///        When buffer size is insufficient for new size, this function allocates new buffer and move previous data
///        to new position while keeps bits aligned at the same position.
/// @param length Size of data.
void BitBuf::ensure_buffer(int64_t offset_delta, int64_t length_delta) {
    int64_t new_offset = offset + offset_delta;
    const uint32_t new_length = length + length_delta; // guarentees length + length_delta >= 0

    data_t *const src_buf = get_buffer();
    data_t *dst_buf = nullptr;
    int64_t memmove_delta = 0; // words distances of moving data up / down
    if (new_offset > 0 && new_length <= capacity * 64) {
        // Case I. Buffer space is sufficient for inplace expansion / shrink.
        // Only clearing expanded space to zeros is required.
        memmove_delta = 0;
        dst_buf = src_buf;
    } else if ((new_offset % 64) + new_length <= capacity * 64) {
        // Case II. Buffer size is sufficient but requires moving words.
        memmove_delta = (new_offset - (new_offset % 64)) / 64;
        new_offset = new_offset % 64; // TODO: reserve one more word?
        dst_buf = src_buf;
    } else {
        // Case III. Allocate new buffer space and move data to new position
        new_offset = heapOffsetWords * 64 + new_offset % 64;
        memmove_delta = (new_offset - offset) / 64;
        auto new_capacity = (new_offset + length + 63) / 64 + 1;
        dst_buf = new data_t[new_capacity];
        this->capacity = new_capacity;
    }
    // II/III Data move: move previous data block to new position
    if (memmove_delta || src_buf != dst_buf) {
        const auto src_start = src_buf + (offset / 64);
        const auto dst_start = dst_buf + (new_offset / 64);
        size_t data_size = ((offset + length - 1) / 64) - (offset / 64) + 1;

        memmove(dst_start, src_start, data_size * sizeof(data_t));

        if (src_buf != dst_buf) {
            if (using_heap_buffer()) delete[] this->heap_buffer;
            this->heap_buffer = dst_buf;
        }
    }
    // I/II/III Finalize: clearing expanded space to zeros
    //    offset expanded: clear lower words
    //    length expanded: clear higher words
    if (offset_delta < 0) {
        const auto clr_start = dst_buf + new_offset / 64;
        const auto clr_last = dst_buf + ((offset - 1) / 64);
        for (auto *ptr = clr_start; ptr <= clr_last - 1; ptr++) *ptr = 0;
        const auto offset_last = offset % 64; // TODO
        *clr_last &= ~((1ull << offset_last) - 1);
    }
    if (length_delta > 0) {
        const auto clr_start = dst_buf + (offset + length) / 64;
        const auto clr_last = dst_buf + ((offset + new_length - 1) / 64);
        const auto offset_start = (offset + length) % 64;
        *clr_start &= (1ull << offset_start) - 1;
        for (auto *ptr = clr_start; ptr <= clr_last - 1; ptr++) *ptr = 0;
    }
    this->offset = new_offset;
    this->length = new_length;
}
