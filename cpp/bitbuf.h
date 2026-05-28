#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>


class BitBuf {
public:
    using data_t = uint64_t;

private:
    static constexpr const int inlineBufferWords = 2;
    static constexpr const uint32_t initialOffsetWords = 1;
    static constexpr const uint32_t initialOffset = initialOffsetWords * 64;
    static constexpr const uint32_t heapOffsetWords = 2;
    static constexpr const uint32_t heapOffset = heapOffsetWords * 64;

    union {
        data_t inline_buffer[inlineBufferWords];
        data_t *heap_buffer = nullptr;
    };
    uint32_t capacity = inlineBufferWords; ///< Buffer capacity in words
    uint32_t length; ///< Bit length of the object
    uint32_t offset; ///< Bit offset of the object

    static uint32_t get_aligned_buffer_size(uint32_t width) {
        return (width + (sizeof(data_t) - 1)) / sizeof(data_t);
    }

    static uint32_t get_bytes_size(uint32_t width) {
        return (width + 7) / 8;
    }

    bool using_heap_buffer() const;

    const data_t *get_buffer() const;

    data_t *get_buffer();

    void get_bits_nocheck(uint32_t pos, uint32_t width, data_t *dst_buf) const;

    void set_bits_nocheck(uint32_t pos, const data_t *src_buffer, uint32_t width);

    void ensure_empty_buffer(uint32_t length);

//    uint8_t *ensure_empty_buffer_aligned_8b(uint32_t length);
    void ensure_buffer(int64_t offset_delta, int64_t length_delta); // no subword shifts and fill new space with zeros

public:
    BitBuf();
    
    BitBuf(const void *buffer, uint32_t width);
    
    ~BitBuf();

    // TODO: copy constructor/move

    BitBuf &assign(const void *buffer, uint32_t width);

    BitBuf &assign_zeros(uint32_t width);

    BitBuf &assign_ones(uint32_t width);

    bool compare(BitBuf &other);

    uint32_t len() const;

    BitBuf &resize(uint32_t width);

    BitBuf &clear();

    int get_bit(uint32_t pos) const;

    // TODO: arg order
    void get_bits(uint32_t pos, uint32_t width, data_t *dst_buf) const;

    BitBuf slice(uint32_t pos, uint32_t width) const;

    BitBuf &set_bit(uint32_t pos, uint32_t value);

    BitBuf &set_bits(uint32_t pos, const data_t *src_buffer, uint32_t width);

    BitBuf &set_ones(uint32_t pos, uint32_t width);

    BitBuf &set_zeros(uint32_t pos, uint32_t width);

    BitBuf &toggle(uint32_t pos, uint32_t width);

    BitBuf &lshift(uint32_t bits);

    BitBuf &rshift(uint32_t bits_);

    BitBuf &append_low(const data_t *value, uint32_t width);

    BitBuf &append_high(const data_t *value, uint32_t width);

    BitBuf &delete_low(uint32_t width);

    BitBuf &delete_high(uint32_t width);

    void pop_low(data_t *dst_buffer, uint32_t width);

    /// dst_buffer should have data_t size
    void pop_high(data_t *dst_buffer, uint32_t width);

    uint32_t width() const;

    uint32_t nbytes() const;

    uint32_t get_offset() const;

    uint8_t *normalize_buffer_8b();

    // uint8_t *normalize_buffer_word();
};

