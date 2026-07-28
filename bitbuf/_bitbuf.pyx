# SPDX-License-Identifier: MIT
# distutils: language = c++
# cython: language_level=3
# cython: c_string_encoding=ascii
#
# CPython binding for the C++ BitBuf class (cpp/bitbuf.cpp). This replaces the
# hand-written C-API module (module.cpp / py_bitbuf.cpp); the C++ layer
# (bitbuf.h/.cpp) is used untouched, and the buffer-extraction / formatting
# helpers live in cpp/utils.{h,cpp}.

from libc.stdint cimport uint8_t, uint32_t, uint64_t
from libcpp.string cimport string

from cpython.object cimport Py_EQ, Py_NE
from cpython.buffer cimport PyBuffer_FillInfo

# BitBuf (C++ class) and the ``bitbuf`` extension type are declared in the
# companion _bitbuf.pxd and are automatically in scope here.


cdef extern from "utils.h":
    cdef cppclass ExtractedBuffer:
        ExtractedBuffer() except +
        const uint64_t *buffer
        uint32_t size
        bint extract(object value, int width)
        bint set_from_bitbuf(BitBuf &other, int width)
        void allocate(uint32_t bit_size)
        uint64_t *get_writable()
    object create_pylong(const void *buffer, uint32_t size)
    string to_hex_string(const uint8_t *ptr, size_t size)


# --------------------------------------------------------------------------
# module-level helpers shared by several methods and protocol slots
# --------------------------------------------------------------------------

cdef int _extract(ExtractedBuffer *buf, object value, int width=-1) except -1:
    # extract()/set_from_bitbuf() set a Python error and return false on failure.
    if isinstance(value, bitbuf):
        if not buf.set_from_bitbuf((<bitbuf> value).bitbuf, width):
            return -1
        return 0
    if not buf.extract(value, width):
        return -1
    return 0


cdef bint _get_bit_common(bitbuf self, int pos):
    if pos < 0:
        pos += <int> self.bitbuf.len()
    if pos < 0 or pos >= <int> self.bitbuf.len():
        raise IndexError("bit range out of range")
    return self.bitbuf.get_bit(<uint32_t> pos)


cdef object _get_bits_common(bitbuf self, int pos, int width):
    if width < 0:
        raise IndexError("width must be non-negative")
    if pos < 0 or pos + width > <int> self.bitbuf.len():
        raise IndexError("bit range out of range")
    cdef ExtractedBuffer buf
    buf.allocate(<uint32_t> width)
    self.bitbuf.get_bits(<uint32_t> pos, buf.size, buf.get_writable())
    return create_pylong(buf.buffer, buf.size)


cdef int _set_bit_common(bitbuf self, int pos, int value) except -1:
    if pos < 0:
        pos += <int> self.bitbuf.len()
    if pos < 0 or pos >= <int> self.bitbuf.len():
        raise IndexError("bit range out of range")
    self.bitbuf.set_bit(<uint32_t> pos, <uint32_t> value)
    return 0


cdef int _set_bits_common(bitbuf self, int pos, int width, object value) except -1:
    cdef ExtractedBuffer buf
    _extract(&buf, value, width)
    if pos < 0 or pos + <int> buf.size > <int> self.bitbuf.len():
        raise IndexError("bit range out of range")
    self.bitbuf.set_bits(<uint32_t> pos, buf.buffer, buf.size)
    return 0


cdef int _lshift(bitbuf self, object bits) except -1:
    cdef int b = bits
    if b < 0:
        raise ValueError("bits must be non-negative")
    if b > 0:
        self.bitbuf.lshift(<uint32_t> b)
    return 0


cdef int _rshift(bitbuf self, object bits) except -1:
    cdef int b = bits
    if b < 0:
        raise ValueError("bits must be non-negative")
    if b > 0:
        self.bitbuf.rshift(<uint32_t> b)
    return 0


cdef bitbuf create_zeros(int width):
    if width < 0:
        raise IndexError("width must be non-negative")
    cdef bitbuf obj = bitbuf.__new__(bitbuf)
    obj.bitbuf.assign_zeros(<uint32_t> width)
    return obj


cdef bitbuf create_ones(int width):
    if width < 0:
        raise IndexError("width must be non-negative")
    cdef bitbuf obj = bitbuf.__new__(bitbuf)
    obj.bitbuf.assign_ones(<uint32_t> width)
    return obj


cdef bitbuf from_buffer(const void *buffer, size_t offset, size_t size):
    cdef bitbuf obj = bitbuf.__new__(bitbuf)
    if size == 0:
        return obj

    cdef size_t first_byte = offset // 8
    cdef size_t unaligned_bits = offset % 8
    cdef uint32_t assign_size = <uint32_t> (size + unaligned_bits)
    cdef const uint64_t *ptr = <const uint64_t *> (<const uint8_t *> buffer + first_byte)

    obj.bitbuf.assign(ptr, assign_size)
    if unaligned_bits > 0:
        obj.bitbuf.delete_low(<uint32_t> unaligned_bits)
    obj.bitbuf.resize(<uint32_t> size)
    return obj


cdef class bitbuf:
    """A fast, mutable bit buffer backed by the C++ BitBuf class."""

    def __init__(self, value=None, width=None):
        cdef ExtractedBuffer buf
        cdef int w = -1
        if value is None:
            return
        if width is not None:
            w = <int> width
        _extract(&buf, value, w)
        self.bitbuf.assign(buf.buffer, buf.size)

    # ---- static / class constructors ----

    @classmethod
    def zeros(cls, width):
        return create_zeros(width)

    @classmethod
    def ones(cls, width):
        return create_ones(width)

    @classmethod
    def from_buffer(cls, buffer, offset, size):
        cdef Py_ssize_t off = offset
        cdef Py_ssize_t sz = size
        if off < 0 or sz < 0:
            raise ValueError("offset and size must be non-negative")

        cdef ExtractedBuffer buf
        _extract(&buf, buffer, -1)

        cdef Py_ssize_t total_bits = <Py_ssize_t> buf.size
        if off > total_bits or sz > total_bits - off:
            raise IndexError("bit range out of range")
        if sz > <Py_ssize_t> 0xFFFFFFFF:
            raise OverflowError("size is too large")

        return from_buffer(buf.buffer, off, sz)

    # ---- dunder / protocol methods ----

    def __len__(self):
        return self.bitbuf.len()

    def __int__(self):
        cdef uint8_t *ptr = self.bitbuf.normalize_buffer_8b()
        return create_pylong(ptr, self.bitbuf.len())

    def __index__(self):
        return self.__int__()

    def __bytes__(self):
        return self.bytes()

    def __repr__(self):
        cdef uint8_t *ptr = self.bitbuf.normalize_buffer_8b()
        cdef uint32_t length = self.bitbuf.len()
        cdef uint32_t n = self.bitbuf.nbytes()
        if length > 128:
            while n and ptr[n - 1] == 0:
                n -= 1
        cdef str hex_value
        cdef str high
        cdef str low
        if n <= 16:
            hex_value = to_hex_string(ptr, n)
        else:
            high = to_hex_string(ptr + n - 8, 8)
            low = to_hex_string(ptr, 8)
            hex_value = "0x" + high[2:] + "..." + low[2:]
        return "bitbuf(len=%d, hex=%s)" % (length, hex_value)

    def __richcmp__(self, other, int op):
        if op != Py_EQ and op != Py_NE:
            return NotImplemented
        if not isinstance(other, bitbuf):
            raise ValueError("Py_NotImplemented")
        cdef bint equal = self.bitbuf.compare((<bitbuf> other).bitbuf)
        if op == Py_NE:
            equal = not equal
        return equal

    def __getitem__(self, key):
        cdef Py_ssize_t start, stop, step
        if isinstance(key, slice):
            start, stop, step = key.indices(self.bitbuf.len())
            if step != 1:
                raise IndexError("step other that 1 is not supported")
            return _get_bits_common(self, <int> start, <int> (stop - start))
        if isinstance(key, int):
            return _get_bit_common(self, <int> key)
        raise TypeError("bit index must be an int or slice")

    def __setitem__(self, key, value):
        cdef Py_ssize_t start, stop, step
        if isinstance(key, slice):
            start, stop, step = key.indices(self.bitbuf.len())
            if step != 1:
                raise IndexError("step other that 1 is not supported")
            _set_bits_common(self, <int> start, stop - start, value)
            return
        if isinstance(key, int):
            _set_bit_common(self, <int> key, <int> value)
            return
        raise TypeError("bit index must be an int or slice")

    def __iadd__(self, other):
        if not isinstance(other, (bytes, bytearray, bitbuf)):
            raise TypeError(
                "unsupported operand type(s) for +=: 'bitbuf' and '%s'" % type(other).__name__)
        cdef ExtractedBuffer buf
        _extract(&buf, other, -1)
        self.bitbuf.append_high(buf.buffer, buf.size)
        return self

    def __ilshift__(self, bits):
        _lshift(self, bits)
        return self

    def __irshift__(self, bits):
        _rshift(self, bits)
        return self

    def __getbuffer__(self, Py_buffer *buffer, int flags):
        cdef uint8_t *ptr = self.bitbuf.normalize_buffer_8b()
        if PyBuffer_FillInfo(buffer, self, <void *> ptr,
                             <Py_ssize_t> self.bitbuf.nbytes(), 1, flags) != 0:
            raise BufferError("could not export bitbuf buffer")

    def __getstate__(self):
        cdef uint8_t *ptr = self.bitbuf.normalize_buffer_8b()
        cdef bytes payload = (<char *> ptr)[:self.bitbuf.nbytes()]
        return (self.bitbuf.len(), payload)

    def __setstate__(self, state):
        width_obj, payload = state
        if not isinstance(payload, bytes):
            raise TypeError("state to restore has unexpected type")
        cdef unsigned long width = width_obj
        if len(payload) != <Py_ssize_t> ((width + 7) // 8):
            raise ValueError("state to restore has inconsistent width and payload size")
        cdef const char *p = payload
        self.bitbuf.assign(<const void *> p, <uint32_t> width)

    # ---- class methods ----

    cpdef bitbuf assign(self, object value=None, int width=-1):
        cdef ExtractedBuffer buf
        if value is not None:
            _extract(&buf, value, width)
            self.bitbuf.assign(buf.buffer, buf.size)
        return self

    cpdef bitbuf resize(self, int width):
        if width < 0:
            raise IndexError("bit range out of range")
        self.bitbuf.resize(<uint32_t> width)
        return self

    cpdef bitbuf clear(self):
        self.bitbuf.clear()
        return self

    cpdef bitbuf clone(self):
        cdef bitbuf obj = bitbuf.__new__(bitbuf)
        obj.bitbuf = self.bitbuf
        return obj

    cpdef int get_bit(self, int pos):
        return _get_bit_common(self, pos)

    cpdef object get_bits(self, int pos, int width):
        return _get_bits_common(self, pos, width)

    cpdef bytes get_bits_as_bytes(self, int pos, int width):
        if width < 0:
            raise IndexError("width must be non-negative")
        if pos < 0 or pos + width > <int> self.bitbuf.len():
            raise IndexError("bit range out of range")
        cdef ExtractedBuffer buf
        buf.allocate(<uint32_t> width)
        self.bitbuf.get_bits(<uint32_t> pos, <uint32_t> width, buf.get_writable())
        return (<char *> buf.get_writable())[:(width + 7) // 8]

    cpdef bytearray get_bits_as_bytearray(self, int pos, int width):
        if width < 0:
            raise IndexError("width must be non-negative")
        if pos < 0 or pos + width > <int> self.bitbuf.len():
            raise IndexError("bit range out of range")
        cdef ExtractedBuffer buf
        buf.allocate(<uint32_t> width)
        self.bitbuf.get_bits(<uint32_t> pos, <uint32_t> width, buf.get_writable())
        return bytearray((<char *> buf.get_writable())[:(width + 7) // 8])

    cpdef bitbuf slice(self, int pos, int width):
        if width < 0:
            raise IndexError("width must be non-negative")
        if pos < 0 or pos + width > <int> self.bitbuf.len():
            raise IndexError("bit range out of range")
        cdef bitbuf obj = bitbuf.__new__(bitbuf)
        obj.bitbuf = self.bitbuf.slice(<uint32_t> pos, <uint32_t> width)
        return obj

    cpdef bitbuf clear_bit(self, int pos):
        _set_bit_common(self, pos, 0)
        return self

    cpdef bitbuf set_bit(self, int pos, int value=1):
        cdef int v = value
        if v != 0 and v != 1:
            raise ValueError("value should be either 0 or 1")
        _set_bit_common(self, pos, v)
        return self

    cpdef bitbuf set_bits(self, int pos, object value, int width=-1):
        _set_bits_common(self, pos, width, value)
        return self

    cpdef bitbuf set_ones(self, int pos, int width):
        if width < 0:
            raise IndexError("width must be non-negative")
        if pos < 0 or pos + width > <int> self.bitbuf.len():
            raise IndexError("bit range out of range")
        self.bitbuf.set_ones(<uint32_t> pos, <uint32_t> width)
        return self

    cpdef bitbuf set_zeros(self, int pos, int width):
        if width < 0:
            raise IndexError("width must be non-negative")
        if pos < 0 or pos + width > <int> self.bitbuf.len():
            raise IndexError("bit range out of range")
        self.bitbuf.set_zeros(<uint32_t> pos, <uint32_t> width)
        return self

    cpdef bitbuf toggle(self, int pos=-1, int width=-1):
        if pos == -1 and width == -1:
            pos = 0
            width = <int> self.bitbuf.len()
        elif width == -1:
            width = 1
        elif pos == -1:
            raise IndexError("invalid arguments: width is specified but pos is not")
        if width < 0:
            raise IndexError("width must be non-negative")
        if pos < 0 or pos + width > <int> self.bitbuf.len():
            raise IndexError("bit range out of range")
        self.bitbuf.toggle(<uint32_t> pos, <uint32_t> width)
        return self

    cpdef bitbuf lshift(self, int bits):
        _lshift(self, bits)
        return self

    cpdef bitbuf rshift(self, int bits):
        _rshift(self, bits)
        return self

    cpdef bitbuf append_low(self, object value, int width=-1):
        if value is None:
            raise TypeError("append_low() missing required argument 'value'")
        cdef ExtractedBuffer buf
        _extract(&buf, value, width)
        self.bitbuf.append_low(buf.buffer, buf.size)
        return self

    cpdef bitbuf append_high(self, object value, int width=-1):
        if value is None:
            raise TypeError("append_high() missing required argument 'value'")
        cdef ExtractedBuffer buf
        _extract(&buf, value, width)
        self.bitbuf.append_high(buf.buffer, buf.size)
        return self

    cpdef bitbuf delete_low(self, int width):
        if width < 0:
            raise IndexError("bit range out of range")
        self.bitbuf.delete_low(<uint32_t> width)
        return self

    cpdef bitbuf delete_high(self, int width):
        if width < 0:
            raise IndexError("bit range out of range")
        self.bitbuf.delete_high(<uint32_t> width)
        return self

    # TODO: pop_low -> size_t?
    cpdef object pop_low(self, int width):
        cdef uint64_t small = 0
        cdef ExtractedBuffer buf
        if width < 0 or width > <int> self.bitbuf.len():
            raise IndexError("bit range out of range")
        if width == 0:
            return 0
        if width <= 64:
            self.bitbuf.pop_low(&small, <uint32_t> width)
            return small
        buf.allocate(<uint32_t> width)
        self.bitbuf.pop_low(buf.get_writable(), buf.size)
        return create_pylong(buf.buffer, buf.size)

    cpdef object pop_high(self, int width):
        cdef uint64_t small = 0
        cdef ExtractedBuffer buf
        if width < 0 or width > <int> self.bitbuf.len():
            raise IndexError("bit range out of range")
        if width == 0:
            return 0
        if width <= 64:
            self.bitbuf.pop_high(&small, <uint32_t> width)
            return small
        buf.allocate(<uint32_t> width)
        self.bitbuf.pop_high(buf.get_writable(), buf.size)
        return create_pylong(buf.buffer, buf.size)

    cpdef bytearray bytearray(self):
        cdef uint8_t *ptr = self.bitbuf.normalize_buffer_8b()
        return bytearray((<char *> ptr)[:(self.bitbuf.len() + 7) // 8])

    cpdef bytes bytes(self):
        cdef uint8_t *ptr = self.bitbuf.normalize_buffer_8b()
        return (<char *> ptr)[:(self.bitbuf.len() + 7) // 8]

    cpdef str hex(self):
        cdef uint8_t *ptr = self.bitbuf.normalize_buffer_8b()
        cdef str result = to_hex_string(ptr, self.bitbuf.nbytes())
        return result

    cpdef object int(self):
        return self.__int__()

    @property
    def width(self):
        return self.bitbuf.width()

    @property
    def nbytes(self):
        return self.bitbuf.nbytes()

    @property
    def offset(self):
        return self.bitbuf.get_offset()
