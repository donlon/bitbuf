#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

#include <nanobind/nanobind.h>
#include "bitbuf.h"

namespace nb = nanobind;

class PyBitBuf {
    using data_t = BitBuf::data_t;
public:
    PyBitBuf();

    PyBitBuf(const nb::object &value, const nb::object &width);

    static PyBitBuf from_int(const nb::object &data, const nb::object &width);

    static PyBitBuf from_bytes(const nb::object &data, const nb::object &width);

    static PyBitBuf zeros(const nb::object &width);

    static PyBitBuf ones(const nb::object &width);

    bool eq(const nb::object &other);

    std::size_t len() const;

    nb::int_ as_int();

    nb::int_ as_index();

    nb::bytes as_bytes();

    std::string repr();

    nb::object getitem(const nb::object &key) const;

    void setitem(const nb::object &key, const nb::object &value);

    PyBitBuf &ilshift(const nb::object &bits);

    PyBitBuf &irshift(const nb::object &bits);

    PyBitBuf &assign(const nb::object &value, const nb::object &width);

    PyBitBuf &resize(const nb::object &width);

    PyBitBuf &clear();

    nb::int_ get_bit(const nb::object &pos) const;

    nb::int_ get_bits(const nb::object &pos_, const nb::object &width_) const;

    nb::bytes get_bits_as_bytes(const nb::object &pos, const nb::object &width) const;

    nb::bytearray get_bits_as_bytearray(const nb::object &pos, const nb::object &width) const;

    PyBitBuf slice(const nb::object &pos, const nb::object &width) const;

    PyBitBuf &set_bit(const nb::object &pos, const nb::object &value);

    PyBitBuf &set_bits(const nb::object &pos, const nb::object &value, const nb::object &width);

    PyBitBuf &set_ones(const nb::object &pos, const nb::object &width);

    PyBitBuf &set_zeros(const nb::object &pos, const nb::object &width);

    PyBitBuf &toggle(const nb::object &pos, const nb::object &width);

    PyBitBuf &lshift(const nb::object &bits);

    PyBitBuf &rshift(const nb::object &bits_);

    PyBitBuf &append_low(const nb::object &value, const nb::object &width);

    PyBitBuf &append_high(const nb::object &value, const nb::object &width);

    PyBitBuf &delete_low(const nb::object &width);

    PyBitBuf &delete_high(const nb::object &width);

    nb::int_ pop_low(const nb::object &width);

    nb::int_ pop_high(const nb::object &width_);

    nb::bytearray as_bytearray();

    nb::bytes bytes();

    std::string hex();

    nb::int_ int_value();

    uint32_t width() const;

    uint32_t nbytes() const;

    uint32_t get_offset() const;

public:
    BitBuf bitbuf{};

};
