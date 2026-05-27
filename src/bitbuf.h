#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

#include <nanobind/nanobind.h>

namespace nb = nanobind;

class BitBuf {
public:
    BitBuf();
    BitBuf(const nb::object &value, const nb::object &width);

    static BitBuf from_int(const nb::object &data, const nb::object &width);
    static BitBuf from_bytes(const nb::object &data, const nb::object &width);
    static BitBuf zeros(const nb::object &width);
    static BitBuf ones(const nb::object &width);

    nb::object eq(const nb::object &other) const;
    std::size_t len() const;
    nb::int_ as_int() const;
    nb::int_ as_index() const;
    nb::bytes as_bytes() const;
    std::string repr() const;
    nb::object getitem(const nb::object &key) const;
    void setitem(const nb::object &key, const nb::object &value);

    BitBuf &ilshift(const nb::object &bits);
    BitBuf &irshift(const nb::object &bits);

    BitBuf &assign(const nb::object &value, const nb::object &width);
    BitBuf &resize(const nb::object &width);
    BitBuf &clear();

    nb::int_ get_bit(const nb::object &pos) const;
    nb::int_ get_bits(const nb::object &pos, const nb::object &width) const;
    nb::bytes get_bits_as_bytes(const nb::object &pos, const nb::object &width) const;
    nb::bytearray get_bits_as_bytearray(const nb::object &pos, const nb::object &width) const;
    BitBuf slice(const nb::object &pos, const nb::object &width) const;

    BitBuf &set_bit(const nb::object &pos, const nb::object &value);
    BitBuf &set_bits(const nb::object &pos, const nb::object &value, const nb::object &width);
    BitBuf &set_ones(const nb::object &pos, const nb::object &width);
    BitBuf &set_zeros(const nb::object &pos, const nb::object &width);
    BitBuf &toggle(const nb::object &pos, const nb::object &width);

    BitBuf &lshift(const nb::object &bits);
    BitBuf &rshift(const nb::object &bits);

    BitBuf &append_low(const nb::object &value, const nb::object &width);
    BitBuf &append_high(const nb::object &value, const nb::object &width);
    BitBuf &delete_low(const nb::object &width);
    BitBuf &delete_high(const nb::object &width);
    nb::int_ pop_low(const nb::object &width);
    nb::int_ pop_high(const nb::object &width);

    nb::bytearray as_bytearray() const;
    nb::bytes bytes() const;
    std::string hex() const;
    nb::int_ int_value() const;

    std::size_t width() const;
    std::size_t nbytes() const;
    std::size_t offset() const;

private:
    static bool is_bytes_like(const nb::handle &value);
    static nb::int_ ensure_int(const nb::object &value);
    static std::size_t ensure_non_negative_size(const nb::object &value, const char *name);
    static nb::int_ mask_for_width(std::size_t width);
    static std::size_t bit_length(const nb::int_ &value);
    static std::string to_hex_string(const nb::int_ &value);

    std::pair<nb::int_, std::size_t> sized_value(const nb::object &value, const nb::object &width) const;
    std::size_t normalize_position(std::ptrdiff_t pos, bool allow_end = false) const;
    std::pair<std::size_t, std::size_t> slice_bounds(const nb::object &slice_obj) const;

    nb::int_ int_py() const;
    void trim();
    void increase_offset(std::size_t bits);
    void decrease_offset(std::size_t bits);

    nb::int_ data_;
    std::size_t offset_;
    std::size_t width_;
};

