// SPDX-License-Identifier: MIT

#pragma once

#include <Python.h>

#include <cstddef>
#include <cstdint>
#include <string>

#include "bitbuf.h"

// Render the little-endian bytes [ptr, ptr+length) as a "0x..." string (no leading
// zeros; "0x0" for an all-zero / empty buffer).
std::string to_hex_string(const uint8_t *ptr, size_t length);

// Build a Python int from the low-endian bytes holding `length` bits.
PyObject *create_pylong(const void *buffer, uint32_t length);

// Extracts a BitBuf-ready buffer from a Python value. Handles int / bytes /
// bytearray / memoryview (bitbuf inputs are handled in the Cython layer, which
// knows the type, via set_from_bitbuf). Owns any scratch storage it allocates.
struct ExtractedBuffer {
    BitBuf::data_t inline_buffer[8]{};
    const BitBuf::data_t *buffer = nullptr;
    uint32_t length = 0;
    bool is_owned = false;
#if PY_MINOR_VERSION < 14
    PyObject *pybytes = nullptr;
#endif
    explicit ExtractedBuffer() = default;

    // Only the move constructor is allowed.
    ExtractedBuffer(const ExtractedBuffer &other) = delete;
    ExtractedBuffer &operator=(const ExtractedBuffer &other) = delete;
    ExtractedBuffer &operator=(ExtractedBuffer &&other) = delete;
    ExtractedBuffer(ExtractedBuffer &&other) noexcept
        : buffer(other.buffer), length(other.length), is_owned(other.is_owned)
#if PY_MINOR_VERSION < 14
          ,
          pybytes(other.pybytes)
#endif
    {
        other.length = 0;
        other.buffer = nullptr;
        other.is_owned = false;
    }

    ~ExtractedBuffer() {
        if (is_owned) delete[] buffer;
#if PY_MINOR_VERSION < 14
        if (pybytes) Py_DecRef(pybytes);
#endif
        buffer = nullptr;
        is_owned = false;
    }

    bool extract(PyObject *value_, int width) {
        if (PyLong_Check(value_) && width < 0) {
            PyErr_SetString(PyExc_ValueError, "width is not specified for int data");
            return false;
        }
        if (width == 0) {
            buffer = inline_buffer;
            length = 0;
            is_owned = false;
            return true;
        }

        if (PyLong_Check(value_)) {
#if PY_MINOR_VERSION >= 14
            Py_ssize_t int_size_ = PyLong_AsNativeBytes(value_, nullptr, 0, Py_ASNATIVEBYTES_LITTLE_ENDIAN);
            if (int_size_ < 0) return false;
            if (int_size_ < width) int_size_ = width;
            auto int_size = static_cast<uint32_t>(int_size_);
            auto buf = int_size < sizeof(inline_buffer) ? inline_buffer : new uint64_t[(int_size + 7) / 8];
            PyLong_AsNativeBytes(value_, buf, int_size_, Py_ASNATIVEBYTES_LITTLE_ENDIAN);
            return assign_buffer(static_cast<uint32_t>(width), width, buf, buf != inline_buffer);
#else
            // TODO: maybe call PyLong_AsLongLongAndOverflow for small numbers
            Py_ssize_t int_size_ = width;
            PyObject *bytes = PyObject_CallMethod( //
                    value_,
                    "to_bytes",
                    "ns",
                    int_size_,
                    "little");
            if (!bytes) {
                PyErr_SetString(PyExc_SystemError, "cannot convert int to buffer");
                return false;
            }

            pybytes = bytes;
            bool result = assign_buffer((uint32_t) PyBytes_GET_SIZE(bytes) * 8,
                                        width,
                                        reinterpret_cast<const BitBuf::data_t *>(PyBytes_AsString(bytes)));
            return result;
#endif
        } else if (PyBytes_Check(value_)) {
            return assign_buffer((uint32_t) PyBytes_GET_SIZE(value_) * 8,
                                 width,
                                 reinterpret_cast<const BitBuf::data_t *>(PyBytes_AsString(value_)));
        } else if (PyByteArray_Check(value_)) {
            return assign_buffer((uint32_t) PyByteArray_GET_SIZE(value_) * 8,
                                 width,
                                 reinterpret_cast<const BitBuf::data_t *>(PyByteArray_AS_STRING(value_)));
        } else if (PyMemoryView_Check(value_)) {
            Py_buffer *buf = PyMemoryView_GET_BUFFER(value_);
            return assign_buffer((uint32_t) buf->len * 8, width, reinterpret_cast<const BitBuf::data_t *>(buf->buf));
        }

        PyErr_SetString(PyExc_TypeError, "value must be a bytes-like object");
        return false;
    }

    // Extract from an already-known bitbuf instance's C++ object (the Cython
    // layer resolves the type and passes the embedded BitBuf here).
    bool set_from_bitbuf(BitBuf &other, int width) {
        if (width == 0) {
            buffer = inline_buffer;
            length = 0;
            is_owned = false;
            return true;
        }
        uint8_t *buf = other.normalize_buffer_8b();
        // TODO: support zero padding
        return assign_buffer(other.len(), width, (BitBuf::data_t *) buf);
    }

    void allocate(uint32_t bit_length) {
        uint32_t byte_length = (bit_length + 7) / 8;
        if (is_owned) delete[] buffer;
        if (byte_length <= sizeof(inline_buffer)) { // FIXME: wrong buffer size
            buffer = inline_buffer;
            is_owned = false;
        } else {
            buffer = new uint64_t[(bit_length + 63) / 64];
            is_owned = true;
        }
        length = bit_length;
    }

    BitBuf::data_t *get_writable() {
        return const_cast<BitBuf::data_t *>(buffer);
    }

private:
    bool assign_buffer(uint32_t buffer_length, int64_t actual_length, const BitBuf::data_t *buffer_,
                       bool is_owned_ = false) {
        if (actual_length >= 0) {
            if (actual_length > buffer_length) {
                PyErr_SetString(PyExc_ValueError, "width is bigger than actual data size");
                return false;
            }
            this->length = static_cast<uint32_t>(actual_length);
        } else {
            this->length = buffer_length;
        }
        this->buffer = buffer_;
        this->is_owned = is_owned_;
        return true;
    }
};
