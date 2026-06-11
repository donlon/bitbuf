// SPDX-License-Identifier: MIT

#include "py_bitbuf.h"

#include <memory>

PyObject *PyBitBuf_get_bit_common(PyObject *self_obj, int pos);
PyObject *PyBitBuf_get_bits_common(PyObject *self_obj, int pos, int width);
PyObject *PyBitBuf_set_bit_common(PyObject *self_obj, int pos, int value);
bool PyBitBuf_set_bits_common(PyObject *self_obj, int pos, PyObject *width, PyObject *value);

static std::string to_hex_string(const uint8_t *ptr, size_t size) {
    static const char *hex_table = "0123456789abcdef";
    if (size == 0) return "0x0";
    std::string str{"0x"};
    str.reserve(2 * size + 4);
    auto i = size - 1;
    bool find_nonzero = false;
    do {
        uint8_t byte = ptr[i];
        if (find_nonzero || byte >> 4) {
            str += hex_table[byte >> 4];
            find_nonzero = true;
        }
        if (find_nonzero || byte & 15) {
            str += hex_table[byte & 15];
            find_nonzero = true;
        }
    } while (i--);
    if (str.length() == 2) str += '0';
    return str;
}

struct ExtractedBuffer {
    BitBuf::data_t inline_buffer[8]{};
    const BitBuf::data_t *buffer = nullptr;
    uint32_t size = 0;
    bool is_owned = false;
#if PY_MINOR_VERSION < 14
    PyObject *pybytes = nullptr;
#endif
    explicit ExtractedBuffer() = default;

    // Only Move assignment operator is allowed
    ExtractedBuffer(const ExtractedBuffer &other) = delete;
    ExtractedBuffer &operator=(const ExtractedBuffer &other) = delete;
    ExtractedBuffer &operator=(ExtractedBuffer &&other) = delete;
    ExtractedBuffer(ExtractedBuffer &&other) noexcept
        : buffer(other.buffer), size(other.size), is_owned(other.is_owned)
#if PY_MINOR_VERSION < 14
          ,
          pybytes(other.pybytes)
#endif
    {
        other.size = 0;
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

    bool extract(PyObject *value_, PyObject *width_) {
        if (PyLong_Check(value_) && width_ == Py_None) {
            PyErr_SetString(PyExc_ValueError, "width is not specified for int data");
            return false;
        }
        int64_t width;
        if (width_ != Py_None) {
            long w = PyLong_AsLong(width_);
            if (w == -1 && PyErr_Occurred()) {
                return false;
            }
            if (w < 0) {
                PyErr_SetString(PyExc_ValueError, "invalid argument 'width'");
                return false;
            }
            width = w;
        } else {
            width = -1;
        }
        if (width == 0) {
            buffer = inline_buffer;
            size = 0;
            is_owned = false;
            return true;
        }

        if (PyBitBuf_Check(value_)) {
            auto *other = PyBitBuf_CAST(value_);
            uint8_t *buf = other->bitbuf.normalize_buffer_8b();
            // TODO: support zero padding
            return assign_buffer(other->bitbuf.len(), width, (BitBuf::data_t *) buf);
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

    void allocate(uint32_t bit_size) {
        uint32_t byte_size = (bit_size + 7) / 8;
        if (is_owned) delete[] buffer;
        if (byte_size <= sizeof(inline_buffer)) {
            buffer = inline_buffer;
            is_owned = false;
        } else {
            buffer = new uint64_t[(bit_size + 63) / 64];
            is_owned = true;
        }
        size = bit_size;
    }

    BitBuf::data_t *get_writable() {
        return const_cast<BitBuf::data_t *>(buffer);
    }

private:
    bool assign_buffer(uint32_t buffer_size, int64_t actual_size, const BitBuf::data_t *buffer_,
                       bool is_owned_ = false) {
        if (actual_size >= 0) {
            if (actual_size > buffer_size) {
                PyErr_SetString(PyExc_ValueError, "width is bigger than actual data size");
                return false;
            }
            this->size = static_cast<uint32_t>(actual_size);
        } else {
            this->size = buffer_size;
        }
        this->buffer = buffer_;
        this->is_owned = is_owned_;
        return true;
    }
};

static PyObject *create_pylong(const void *buffer, uint32_t size) {
#if PY_MINOR_VERSION >= 14
    return PyLong_FromNativeBytes(reinterpret_cast<const char *>(buffer),
                                  (size + 7) / 8,
                                  Py_ASNATIVEBYTES_LITTLE_ENDIAN | Py_ASNATIVEBYTES_UNSIGNED_BUFFER);
#else
    // TODO: use _PyLong_FromByteArray for version < 3.13 ?
    if (size <= 64) {
        uint64_t value = reinterpret_cast<const uint64_t *>(buffer)[0] & ((1ull << size) - 1);
        return PyLong_FromUnsignedLongLong(value);
    } else {
        PyObject *bytes = PyBytes_FromStringAndSize((const char *) buffer, (size + 7) / 8);
        auto obj = PyObject_CallMethod((PyObject *) &PyLong_Type, "from_bytes", "Os", bytes, "little");
        Py_DecRef(bytes);
        return obj;
    }
#endif
}
PyObject *PyBitBuf_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    (void) args;
    (void) kwds;
    auto *self = PyBitBuf_CAST(type->tp_alloc(type, 0));
    if (!self) {
        return nullptr;
    }

    new (&self->bitbuf) BitBuf();
    return reinterpret_cast<PyObject *>(self);
}

void PyBitBuf_dealloc(PyBitBufObject *self) {
    self->bitbuf.~BitBuf();
    Py_TYPE(self)->tp_free(reinterpret_cast<PyObject *>(self));
}

int PyBitBuf_init(PyBitBufObject *self, PyObject *args, PyObject *kwds) {
    static const char *kwlist[] = {"value", "width", nullptr};
    PyObject *value = Py_None;
    PyObject *width = Py_None; // TODO: parse int

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|OO:bitbuf", const_cast<char **>(kwlist), &value, &width)) {
        return -1;
    }
    if (value == Py_None) {
        // Empty
        return 0;
    }

    ExtractedBuffer buf{};
    bool ok = buf.extract(value, width);
    if (!ok) return -1;
    self->bitbuf.assign(buf.buffer, buf.size);
    return 0;
}

static bool parse_int(PyObject *obj, int *out) {
    long value = PyLong_AsLong(obj);
    if (value == -1 && PyErr_Occurred()) {
        return false;
    }
    *out = static_cast<int>(value);
    return true;
}

PyObject *PyBitBuf_zeros(PyObject *cls, PyObject *width_) {
    int width = 0;
    if (!parse_int(width_, &width)) { // TODO: PyLong_AsUnsignedLong
        return nullptr;
    }
    if (width < 0) {
        PyErr_SetString(PyExc_IndexError, "width must be non-negative");
        return nullptr;
    }

    PyObject *obj = PyObject_CallNoArgs(cls);
    if (obj == nullptr) {
        return nullptr;
    }
    PyBitBuf_CAST(obj)->bitbuf.assign_zeros(static_cast<uint32_t>(width));
    return obj;
}

PyObject *PyBitBuf_ones(PyObject *cls, PyObject *width_) {
    int width = 0;
    if (!parse_int(width_, &width)) { // TODO: PyLong_AsUnsignedLong
        return nullptr;
    }
    if (width < 0) {
        PyErr_SetString(PyExc_IndexError, "width must be non-negative");
        return nullptr;
    }

    PyObject *obj = PyObject_CallNoArgs(cls);
    if (obj == nullptr) {
        return nullptr;
    }
    PyBitBuf_CAST(obj)->bitbuf.assign_ones(static_cast<uint32_t>(width));
    return obj;
}

PyObject *PyBitBuf_richcompare(PyObject *self_obj, PyObject *other_obj, int op) {
    if (op != Py_EQ && op != Py_NE) {
        Py_RETURN_NOTIMPLEMENTED;
    }
    if (!PyBitBuf_Check(other_obj)) {
        PyErr_SetString(PyExc_ValueError, "Py_NotImplemented");
        return nullptr;
    }
    auto *self = PyBitBuf_CAST(self_obj);
    auto *rhs = PyBitBuf_CAST(other_obj);
    bool equal = self->bitbuf.compare(rhs->bitbuf) ^ (op == Py_NE);
    return equal ? Py_True : Py_False;
}

Py_ssize_t PyBitBuf_len(PyObject *self_obj) {
    auto *self = PyBitBuf_CAST(self_obj);
    return static_cast<Py_ssize_t>(self->bitbuf.len()); // TODO: checks all Py_ssize_t
}

PyObject *PyBitBuf_as_int(PyObject *self_obj, PyObject *ignored) {
    (void) ignored;
    auto *self = PyBitBuf_CAST(self_obj);
    uint8_t *ptr = self->bitbuf.normalize_buffer_8b();
    return create_pylong(ptr, self->bitbuf.len());
}

PyObject *PyBitBuf_as_index(PyObject *self_obj) {
    return PyBitBuf_as_int(self_obj, nullptr);
}

PyObject *PyBitBuf_as_bytes(PyObject *self_obj, PyObject *ignored) {
    (void) ignored;
    return PyBitBuf_bytes_method(self_obj, nullptr);
}

PyObject *PyBitBuf_getstate(PyObject *self_) {
    auto *self = PyBitBuf_CAST(self_);
    uint8_t *ptr = self->bitbuf.normalize_buffer_8b();

    // Hack of appending bit size before the payload
    Py_ssize_t buffer_size = (self->bitbuf.len() + 7) / 8 + 4;
    PyObject *state = PyBytes_FromStringAndSize(reinterpret_cast<const char *>(ptr - 4), buffer_size);
    char *bytes_ptr = PyBytes_AS_STRING(state);
    *reinterpret_cast<uint32_t *>(bytes_ptr) = self->bitbuf.len();
    return state;
}

PyObject *PyBitBuf_setstate(PyObject *self_, PyObject *state) {
    auto *self = PyBitBuf_CAST(self_);
    if (!PyBytes_Check(state)) {
        PyErr_SetString(PyExc_TypeError, "state to restore has unexpected type");
        return nullptr;
    }
    auto state_size = PyBytes_GET_SIZE(state);
    if (state_size < 4) {
        PyErr_SetString(PyExc_ValueError, "state to restore has invalid size");
        return nullptr;
    }
    auto state_buf = PyBytes_AS_STRING(state);
    auto buf_size = *reinterpret_cast<uint32_t *>(state_buf);
    if (4 + (buf_size + 7) / 8 != state_size) {
        PyErr_SetString(PyExc_ValueError, "state to restore has invalid size");
        return nullptr;
    }
    self->bitbuf.assign(state_buf + 4, buf_size);
    return Py_None;
}

PyObject *PyBitBuf_repr(PyObject *self_obj) {
    auto *self = PyBitBuf_CAST(self_obj);
    uint8_t *ptr = self->bitbuf.normalize_buffer_8b();
    std::string hex_value;
    uint32_t len = self->bitbuf.nbytes();
    if (self->bitbuf.len() > 128) {
        for (; len; len--) {
            if (ptr[len - 1]) break;
        }
    }
    if (len <= 16) {
        hex_value = to_hex_string(ptr, len);
    } else {
        std::string high_hex = to_hex_string(ptr + len - 8, 8).substr(2);
        std::string low_hex = to_hex_string(ptr, 8).substr(2);
        hex_value = "0x" + high_hex + "..." + low_hex;
    }
    std::string repr = "bitbuf(len=" + std::to_string(self->bitbuf.len()) + ", hex=" + hex_value + ")";
    return PyUnicode_FromStringAndSize(repr.data(), static_cast<Py_ssize_t>(repr.size()));
}

PyObject *PyBitBuf_getitem(PyObject *self_, PyObject *key) {
    auto *self = PyBitBuf_CAST(self_);

    if (PySlice_Check(key)) {
        Py_ssize_t start = 0;
        Py_ssize_t stop = 0;
        Py_ssize_t step = 0;
        Py_ssize_t slice_length = 0;
        if (PySlice_GetIndicesEx(
                    key, static_cast<Py_ssize_t>(self->bitbuf.len()), &start, &stop, &step, &slice_length) < 0) {
            return nullptr;
        }
        if (step != 1) {
            PyErr_SetString(PyExc_IndexError, "step other that 1 is not supported");
            return nullptr;
        }
        return PyBitBuf_get_bits_common(self_, static_cast<int>(start), static_cast<int>(stop - start));
        // return get_bits(nb::int_(start), nb::int_(slice_length)); // TODO: avoid wrapping python object
    }
    if (PyLong_Check(key)) {
        int pos = 0;
        if (!parse_int(key, &pos)) {
            return nullptr;
        }
        return PyBitBuf_get_bit_common(self_, pos);
    }
    PyErr_SetString(PyExc_TypeError, "bit index must be an int or slice");
    return nullptr;
}

int PyBitBuf_setitem(PyObject *self_obj, PyObject *key, PyObject *value_) {
    auto *self = PyBitBuf_CAST(self_obj);

    if (PySlice_Check(key)) {
        Py_ssize_t start = 0;
        Py_ssize_t stop = 0;
        Py_ssize_t step = 0;
        Py_ssize_t slice_length = 0;
        if (PySlice_GetIndicesEx(
                    key, static_cast<Py_ssize_t>(self->bitbuf.len()), &start, &stop, &step, &slice_length) < 0) {
            return -1;
        }
        if (step != 1) {
            PyErr_SetString(PyExc_IndexError, "step other that 1 is not supported");
            return -1;
        }
        int pos = static_cast<int>(start);
        PyObject *slice_width = PyLong_FromSsize_t(slice_length);
        if (slice_width == nullptr) {
            return -1;
        }
        bool ok = PyBitBuf_set_bits_common(self_obj, pos, slice_width, value_);
        Py_DECREF(slice_width); // TODO: no this object
        if (!ok) {
            return -1;
        }
        return 0;
    }
    if (PyLong_Check(key)) {
        int pos = 0;
        int value = 0;
        if (!parse_int(key, &pos) || !parse_int(value_, &value)) {
            return -1;
        }
        PyBitBuf_set_bit_common(self_obj, pos, value);
        return 0;
    }

    PyErr_SetString(PyExc_TypeError, "bit index must be an int or slice");
    return -1;
}

static PyObject *return_self(PyObject *self) {
    Py_INCREF(self);
    return self;
}

PyObject *PyBitBuf_ilshift(PyObject *self_obj, PyObject *arg) {
    auto *self = PyBitBuf_CAST(self_obj);
    int bits = 0;
    if (!parse_int(arg, &bits)) {
        return nullptr;
    }
    if (bits < 0) {
        PyErr_SetString(PyExc_ValueError, "bits must be non-negative");
        return nullptr;
    }
    if (bits > 0) {
        self->bitbuf.lshift(static_cast<uint32_t>(bits));
    }
    return return_self(self_obj);
}

PyObject *PyBitBuf_irshift(PyObject *self_obj, PyObject *arg) {
    auto *self = PyBitBuf_CAST(self_obj);
    int bits = 0;
    if (!parse_int(arg, &bits)) {
        return nullptr;
    }
    if (bits < 0) {
        PyErr_SetString(PyExc_ValueError, "bits must be non-negative");
        return nullptr;
    }
    if (bits > 0) {
        self->bitbuf.rshift(static_cast<uint32_t>(bits));
    }
    return return_self(self_obj);
}

PyObject *PyBitBuf_iadd(PyObject *self_obj, PyObject *arg) {
    if (!(PyBytes_Check(arg) || PyByteArray_Check(arg) || PyBitBuf_Check(arg))) {
        PyErr_Format(PyExc_TypeError, "unsupported operand type(s) for +=: 'bitbuf' and '%s'", Py_TYPE(arg)->tp_name);
        return nullptr;
    }
    ExtractedBuffer buf;
    if (!buf.extract(arg, Py_None)) {
        return nullptr;
    }
    PyBitBuf_CAST(self_obj)->bitbuf.append_high(buf.buffer, buf.size);
    return return_self(self_obj);
}

PyObject *PyBitBuf_assign(PyObject *self_obj, PyObject *args, PyObject *kwargs) {
    if (PyBitBuf_init(PyBitBuf_CAST(self_obj), args, kwargs) != 0) {
        // PyErr_SetString(PyExc_IndexError, "failed to assign value to bitbuf object");
        return nullptr;
    }
    return return_self(self_obj);
}

PyObject *PyBitBuf_resize(PyObject *self_obj, PyObject *width_) {
    int width = 0;
    if (!parse_int(width_, &width)) {
        return nullptr;
    }
    if (width < 0) {
        PyErr_SetString(PyExc_IndexError, "bit range out of range");
        return nullptr;
    }

    PyBitBuf_CAST(self_obj)->bitbuf.resize(static_cast<uint32_t>(width));
    return return_self(self_obj);
}

PyObject *PyBitBuf_clear(PyObject *self_obj, PyObject *ignored) {
    (void) ignored;
    PyBitBuf_CAST(self_obj)->bitbuf.clear();
    return return_self(self_obj);
}

PyObject *PyBitBuf_clone(PyObject *self_obj) {
    PyObject *obj = PyObject_CallNoArgs(reinterpret_cast<PyObject *>(&PyBitBufType));
    if (obj == nullptr) {
        return nullptr;
    }
    PyBitBuf_CAST(obj)->bitbuf = PyBitBuf_CAST(self_obj)->bitbuf;
    return obj;
}

PyObject *PyBitBuf_get_bit(PyObject *self_obj, PyObject *pos_) {
    int pos = 0;
    if (!parse_int(pos_, &pos)) {
        return nullptr;
    }
    return PyBitBuf_get_bit_common(self_obj, pos);
}

PyObject *PyBitBuf_get_bit_common(PyObject *self_obj, int pos) {
    auto *self = PyBitBuf_CAST(self_obj);
    if (pos < 0) pos += static_cast<int>(self->bitbuf.len());
    if (pos < 0 || pos >= static_cast<int>(self->bitbuf.len())) {
        PyErr_SetString(PyExc_IndexError, "bit range out of range");
        return nullptr;
    }
    return PyLong_FromLong(self->bitbuf.get_bit(static_cast<uint32_t>(pos)));
}

PyObject *PyBitBuf_get_bits(PyObject *self_obj, PyObject *args, PyObject *kwargs) {
    static const char *kwlist[] = {"pos", "width", nullptr};
    PyObject *pos_obj = nullptr;
    PyObject *width_obj = nullptr;
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO:get_bits", const_cast<char **>(kwlist), &pos_obj, &width_obj)) {
        return nullptr;
    }
    int pos = 0;
    int width = 0;
    if (!parse_int(pos_obj, &pos) || !parse_int(width_obj, &width)) {
        return nullptr;
    }
    return PyBitBuf_get_bits_common(self_obj, pos, width);
}

PyObject *PyBitBuf_get_bits_common(PyObject *self_obj, int pos, int width) {
    auto *self = PyBitBuf_CAST(self_obj);
    if (width < 0) {
        PyErr_SetString(PyExc_IndexError, "width must be non-negative");
        return nullptr;
    }
    if (pos < 0 || pos + width > static_cast<int>(self->bitbuf.len())) {
        PyErr_SetString(PyExc_IndexError, "bit range out of range");
        return nullptr;
    }
    ExtractedBuffer buf{};
    buf.allocate(static_cast<uint32_t>(width));
    self->bitbuf.get_bits(static_cast<uint32_t>(pos), buf.size, buf.get_writable());
    return create_pylong(buf.buffer, buf.size);
}

PyObject *PyBitBuf_get_bits_as_bytes(PyObject *self_obj, PyObject *args, PyObject *kwargs) {
    auto *self = PyBitBuf_CAST(self_obj);
    static const char *kwlist[] = {"pos", "width", nullptr};
    PyObject *pos_obj = Py_None;
    PyObject *width_obj = Py_None;
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO:get_bits", const_cast<char **>(kwlist), &pos_obj, &width_obj)) {
        return nullptr;
    }
    int pos = 0;
    int width = 0;
    if (!parse_int(pos_obj, &pos) || !parse_int(width_obj, &width)) {
        return nullptr;
    }
    if (width < 0) {
        PyErr_SetString(PyExc_IndexError, "width must be non-negative");
        return nullptr;
    }
    if (pos < 0 || pos + width > static_cast<int>(self->bitbuf.len())) {
        PyErr_SetString(PyExc_IndexError, "bit range out of range");
        return nullptr;
    }

    const auto buffer_size = (static_cast<uint32_t>(width) + 63) / 64;
    std::unique_ptr<BitBuf::data_t[]> buf(new BitBuf::data_t[buffer_size]);
    self->bitbuf.get_bits(static_cast<uint32_t>(pos), static_cast<uint32_t>(width), buf.get());
    return PyBytes_FromStringAndSize(reinterpret_cast<const char *>(buf.get()), (width + 7) / 8);
}

PyObject *PyBitBuf_get_bits_as_bytearray(PyObject *self_obj, PyObject *args, PyObject *kwargs) {
    auto *self = PyBitBuf_CAST(self_obj);
    static const char *kwlist[] = {"pos", "width", nullptr};
    PyObject *pos_obj = nullptr;
    PyObject *width_obj = nullptr;
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO:get_bits", const_cast<char **>(kwlist), &pos_obj, &width_obj)) {
        return nullptr;
    }
    int pos = 0;
    int width = 0;
    if (!parse_int(pos_obj, &pos) || !parse_int(width_obj, &width)) {
        return nullptr;
    }
    if (width < 0) {
        PyErr_SetString(PyExc_IndexError, "width must be non-negative");
        return nullptr;
    }
    if (pos < 0 || pos + width > static_cast<int>(self->bitbuf.len())) {
        PyErr_SetString(PyExc_IndexError, "bit range out of range");
        return nullptr;
    }

    PyObject *ba = PyByteArray_FromStringAndSize(nullptr, 0);
    if (ba == nullptr) {
        return nullptr;
    }
    uint64_t i = (static_cast<uint32_t>(width) + 63) / 64 * sizeof(BitBuf::data_t);
    if (PyByteArray_Resize(ba, static_cast<Py_ssize_t>(i)) < 0) {
        Py_DECREF(ba);
        return nullptr;
    }
    self->bitbuf.get_bits(static_cast<uint32_t>(pos),
                          static_cast<uint32_t>(width),
                          reinterpret_cast<BitBuf::data_t *>(PyByteArray_AsString(ba)));
    if (PyByteArray_Resize(ba, (width + 7) / 8) < 0) {
        Py_DECREF(ba);
        return nullptr;
    }
    return ba;
}

PyObject *PyBitBuf_slice(PyObject *self_obj, PyObject *args, PyObject *kwargs) {
    static const char *kwlist[] = {"pos", "width", nullptr};
    PyObject *pos_obj = nullptr;
    PyObject *width_obj = nullptr;
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO:slice", const_cast<char **>(kwlist), &pos_obj, &width_obj)) {
        return nullptr;
    }

    int pos = 0;
    int width = 0;
    if (!parse_int(pos_obj, &pos) || !parse_int(width_obj, &width)) {
        return nullptr;
    }

    auto *self = PyBitBuf_CAST(self_obj);
    if (width < 0) {
        PyErr_SetString(PyExc_IndexError, "width must be non-negative");
        return nullptr;
    }
    if (pos < 0 || pos + width > static_cast<int>(self->bitbuf.len())) {
        PyErr_SetString(PyExc_IndexError, "bit range out of range");
        return nullptr;
    }

    // TODO: directly create from type
    PyObject *obj = PyObject_CallNoArgs(reinterpret_cast<PyObject *>(&PyBitBufType));
    if (obj == nullptr) {
        return nullptr;
    }
    PyBitBuf_CAST(obj)->bitbuf = self->bitbuf.slice(static_cast<uint32_t>(pos), static_cast<uint32_t>(width));
    return obj;
}

PyObject *PyBitBuf_set_bit(PyObject *self_obj, PyObject *args, PyObject *kwargs) {
    static const char *kwlist[] = {"pos", "value", nullptr};
    PyObject *pos_obj = nullptr;
    PyObject *value_obj = nullptr;
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|O:set_bit", const_cast<char **>(kwlist), &pos_obj, &value_obj)) {
        return nullptr;
    }

    int pos = 0;
    int value = 0;
    if (!parse_int(pos_obj, &pos)) {
        return nullptr;
    }
    if(!value_obj) {
        value = 1;
    } else if (!parse_int(value_obj, &value)) {
        return nullptr;
    }
    if (value != 0 && value != 1){
        PyErr_SetString(PyExc_ValueError, "value should be either 0 or 1");
        return nullptr;
    }
    return PyBitBuf_set_bit_common(self_obj, pos, value);
}

PyObject *PyBitBuf_set_bit_common(PyObject *self_obj, int pos, int value) {
    auto *self = PyBitBuf_CAST(self_obj);
    if (pos < 0) {
        pos += static_cast<int>(self->bitbuf.len());
    }
    if (pos < 0 || pos >= static_cast<int>(self->bitbuf.len())) {
        PyErr_SetString(PyExc_IndexError, "bit range out of range");
        return nullptr;
    }

    self->bitbuf.set_bit(static_cast<uint32_t>(pos), static_cast<uint32_t>(value));
    return return_self(self_obj);
}

PyObject *PyBitBuf_set_bits(PyObject *self_obj, PyObject *args, PyObject *kwargs) {
    static const char *kwlist[] = {"pos", "value", "width", nullptr};
    PyObject *pos_obj = nullptr;
    PyObject *value = nullptr;
    PyObject *width = Py_None;
    if (!PyArg_ParseTupleAndKeywords(
                args, kwargs, "OO|O:set_bits", const_cast<char **>(kwlist), &pos_obj, &value, &width)) {
        return nullptr;
    }

    int pos = 0;
    if (!parse_int(pos_obj, &pos)) {
        return nullptr;
    }
    if (!PyBitBuf_set_bits_common(self_obj, pos, width, value)) {
        return nullptr;
    }
    return return_self(self_obj);
}

bool PyBitBuf_set_bits_common(PyObject *self_obj, int pos, PyObject *width, PyObject *value) {
    auto *self = PyBitBuf_CAST(self_obj);

    ExtractedBuffer buf{};
    if (!buf.extract(value, width)) {
        return false;
    }

    if (pos < 0 || pos + static_cast<int>(buf.size) > static_cast<int>(self->bitbuf.len())) {
        // TODO: check before ExtractedBuffer::extract
        PyErr_SetString(PyExc_IndexError, "bit range out of range");
        return false;
    }

    self->bitbuf.set_bits(static_cast<uint32_t>(pos), buf.buffer, buf.size);
    return true;
}

PyObject *PyBitBuf_set_ones(PyObject *self_obj, PyObject *args, PyObject *kwargs) {
    auto *self = PyBitBuf_CAST(self_obj);
    static const char *kwlist[] = {"pos", "width", nullptr};
    PyObject *pos_obj = Py_None;
    PyObject *width_obj = Py_None;
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO:set_ones", const_cast<char **>(kwlist), &pos_obj, &width_obj)) {
        return nullptr;
    }

    int pos = 0;
    int width = 0;
    if (!parse_int(pos_obj, &pos) || !parse_int(width_obj, &width)) {
        return nullptr;
    }

    if (width < 0) {
        PyErr_SetString(PyExc_IndexError, "width must be non-negative");
        return nullptr;
    }
    int end = pos + width;
    if (pos < 0 || end > static_cast<int>(self->bitbuf.len())) {
        PyErr_SetString(PyExc_IndexError, "bit range out of range");
        return nullptr;
    }

    self->bitbuf.set_ones(static_cast<uint32_t>(pos), static_cast<uint32_t>(width));
    return return_self(self_obj);
}

PyObject *PyBitBuf_set_zeros(PyObject *self_obj, PyObject *args, PyObject *kwargs) {
    static const char *kwlist[] = {"pos", "width", nullptr};
    PyObject *pos_obj = nullptr;
    PyObject *width_obj = nullptr;
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO:set_zeros", const_cast<char **>(kwlist), &pos_obj, &width_obj)) {
        return nullptr;
    }

    int pos = 0;
    int width = 0;
    if (!parse_int(pos_obj, &pos) || !parse_int(width_obj, &width)) {
        return nullptr;
    }
    auto *self = PyBitBuf_CAST(self_obj);
    if (width < 0) {
        PyErr_SetString(PyExc_IndexError, "width must be non-negative");
        return nullptr;
    }
    if (pos < 0 || pos + width > static_cast<int>(self->bitbuf.len())) {
        PyErr_SetString(PyExc_IndexError, "bit range out of range");
        return nullptr;
    }

    self->bitbuf.set_zeros(static_cast<uint32_t>(pos), static_cast<uint32_t>(width));
    return return_self(self_obj);
}

PyObject *PyBitBuf_toggle(PyObject *self_obj, PyObject *args, PyObject *kwargs) {
    auto *self = PyBitBuf_CAST(self_obj);
    static const char *kwlist[] = {"pos", "width", nullptr};
    PyObject *pos_obj = Py_None;
    PyObject *width_obj = Py_None;
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|OO:toggle", const_cast<char **>(kwlist), &pos_obj, &width_obj)) {
        return nullptr;
    }

    // TODO: pos == None
    int pos = 0;
    int width = 0;
    if (pos_obj == Py_None && width_obj == Py_None) {
        pos = 0;
        width = (int) self->bitbuf.len();
    } else if (width_obj == Py_None) {
        width = 1;
    }
    if (pos_obj != Py_None && !parse_int(pos_obj, &pos)) return nullptr;
    if (width_obj != Py_None && !parse_int(width_obj, &width)) return nullptr;

    if (width < 0) {
        PyErr_SetString(PyExc_IndexError, "width must be non-negative");
        return nullptr;
    }
    if (pos < 0 || pos + width > static_cast<int>(self->bitbuf.len())) {
        PyErr_SetString(PyExc_IndexError, "bit range out of range");
        return nullptr;
    }

    self->bitbuf.toggle(static_cast<uint32_t>(pos), static_cast<uint32_t>(width));
    return return_self(self_obj);
}

PyObject *PyBitBuf_lshift(PyObject *self_obj, PyObject *bits_) {
    return PyBitBuf_ilshift(self_obj, bits_);
}

PyObject *PyBitBuf_rshift(PyObject *self_obj, PyObject *bits_) {
    return PyBitBuf_irshift(self_obj, bits_);
}

PyObject *PyBitBuf_append_low(PyObject *self_obj, PyObject *args, PyObject *kwargs) {
    static const char *kwlist[] = {"value", "width", nullptr};
    PyObject *value = nullptr;
    PyObject *width = Py_None;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|O:append_low", const_cast<char **>(kwlist), &value, &width)) {
        return nullptr;
    }

    ExtractedBuffer buf;
    if (!buf.extract(value, width)) {
        return nullptr;
    }
    PyBitBuf_CAST(self_obj)->bitbuf.append_low(buf.buffer, buf.size);
    return return_self(self_obj);
}

PyObject *PyBitBuf_append_high(PyObject *self_obj, PyObject *args, PyObject *kwargs) {
    static const char *kwlist[] = {"value", "width", nullptr};
    PyObject *value = nullptr;
    PyObject *width = Py_None;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|O:append_high", const_cast<char **>(kwlist), &value, &width)) {
        return nullptr;
    }

    ExtractedBuffer buf;
    if (!buf.extract(value, width)) {
        return nullptr;
    }
    PyBitBuf_CAST(self_obj)->bitbuf.append_high(buf.buffer, buf.size);
    return return_self(self_obj);
}

PyObject *PyBitBuf_delete_low(PyObject *self_obj, PyObject *width_) {
    auto *self = PyBitBuf_CAST(self_obj);
    int width = 0;
    if (!parse_int(width_, &width)) {
        return nullptr;
    }
    if (width < 0) {
        PyErr_SetString(PyExc_IndexError, "bit range out of range");
        return nullptr;
    }

    self->bitbuf.delete_low(static_cast<uint32_t>(width));
    return return_self(self_obj);
}

PyObject *PyBitBuf_delete_high(PyObject *self_obj, PyObject *width_) {
    auto *self = PyBitBuf_CAST(self_obj);
    int width = 0;
    if (!parse_int(width_, &width)) {
        return nullptr;
    }
    if (width < 0) {
        PyErr_SetString(PyExc_IndexError, "bit range out of range");
        return nullptr;
    }

    self->bitbuf.delete_high(static_cast<uint32_t>(width));
    return return_self(self_obj);
}

PyObject *PyBitBuf_pop_low(PyObject *self_obj, PyObject *width_) {
    auto *self = PyBitBuf_CAST(self_obj);
    int width = 0;
    if (!parse_int(width_, &width)) {
        return nullptr;
    }

    if (width < 0 || width > static_cast<int>(self->bitbuf.len())) {
        PyErr_SetString(PyExc_IndexError, "bit range out of range");
        return nullptr;
    }
    if (width == 0) {
        return PyLong_FromLong(0);
    }
    if (width <= 64) {
        BitBuf::data_t buf = 0;
        self->bitbuf.pop_low(&buf, static_cast<uint32_t>(width));
        return PyLong_FromUnsignedLongLong(buf);
    }

    ExtractedBuffer buf{};
    buf.allocate(static_cast<uint32_t>(width));
    self->bitbuf.pop_low(buf.get_writable(), buf.size);
    return create_pylong(buf.buffer, buf.size);
}

PyObject *PyBitBuf_pop_high(PyObject *self_obj, PyObject *width_) {
    auto *self = PyBitBuf_CAST(self_obj);
    int width = 0;
    if (!parse_int(width_, &width)) {
        return nullptr;
    }

    if (width < 0 || width > static_cast<int>(self->bitbuf.len())) {
        PyErr_SetString(PyExc_IndexError, "bit range out of range");
        return nullptr;
    }
    if (width == 0) {
        return PyLong_FromLong(0);
    }
    if (width <= 64) {
        BitBuf::data_t buf = 0;
        self->bitbuf.pop_high(&buf, static_cast<uint32_t>(width));
        return PyLong_FromUnsignedLongLong(buf);
    }

    ExtractedBuffer buf{};
    buf.allocate(static_cast<uint32_t>(width));
    self->bitbuf.pop_high(buf.get_writable(), buf.size);
    return create_pylong(buf.buffer, buf.size);
}

PyObject *PyBitBuf_bytearray(PyObject *self_obj, PyObject *ignored) {
    (void) ignored;
    auto *self = PyBitBuf_CAST(self_obj);
    uint8_t *ptr = self->bitbuf.normalize_buffer_8b();
    return PyByteArray_FromStringAndSize(reinterpret_cast<const char *>(ptr),
                                         static_cast<Py_ssize_t>((self->bitbuf.len() + 7) / 8));
}

PyObject *PyBitBuf_bytes_method(PyObject *self_obj, PyObject *ignored) {
    (void) ignored;
    auto *self = PyBitBuf_CAST(self_obj);
    uint8_t *ptr = self->bitbuf.normalize_buffer_8b();
    return PyBytes_FromStringAndSize(reinterpret_cast<const char *>(ptr),
                                     static_cast<Py_ssize_t>((self->bitbuf.len() + 7) / 8));
}

PyObject *PyBitBuf_hex(PyObject *self_obj, PyObject *ignored) {
    (void) ignored;
    auto *self = PyBitBuf_CAST(self_obj);
    uint8_t *ptr = self->bitbuf.normalize_buffer_8b();
    std::string hex = to_hex_string(ptr, self->bitbuf.nbytes());
    return PyUnicode_FromStringAndSize(hex.data(), static_cast<Py_ssize_t>(hex.size()));
}

PyObject *PyBitBuf_int_method(PyObject *self_obj, PyObject *ignored) {
    (void) ignored;
    return PyBitBuf_as_int(self_obj, nullptr);
}

PyObject *PyBitBuf_get_width(PyObject *self_obj, void *closure) {
    (void) closure;
    auto *self = PyBitBuf_CAST(self_obj);
    return PyLong_FromUnsignedLong(self->bitbuf.width());
}

PyObject *PyBitBuf_get_nbytes(PyObject *self_obj, void *closure) {
    (void) closure;
    auto *self = PyBitBuf_CAST(self_obj);
    return PyLong_FromUnsignedLong(self->bitbuf.nbytes());
}

PyObject *PyBitBuf_get_offset(PyObject *self_obj, void *closure) {
    (void) closure;
    auto *self = PyBitBuf_CAST(self_obj);
    return PyLong_FromUnsignedLong(self->bitbuf.get_offset());
}
