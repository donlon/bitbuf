# SPDX-License-Identifier: MIT
# distutils: language = c++
#
# Cython interface tests. These run as a real compiled module: conftest.py
# cythonizes and builds this file in place, then collects the ``test_*``
# functions below. The module compiles cpp/bitbuf.cpp into itself (the
# header-distributed model) and uses the public bitbuf Cython interface
# (bitbuf/__init__.pxd re-exporting bitbuf/_bitbuf.pxd).

from libc.stdint cimport uint32_t, uint64_t
from libc.stdlib cimport malloc, free

from bitbuf cimport BitBuf, bitbuf as PyBitBuf

import bitbuf
import pytest


# --------------------------------------------------------------------------
# helpers
# --------------------------------------------------------------------------

cdef object _to_py(BitBuf *src):
    # Hand a C++ BitBuf back to Python by copy-assigning into a fresh object.
    cdef PyBitBuf o = PyBitBuf.__new__(PyBitBuf)
    o.bitbuf = src[0]
    return o


cdef BitBuf *_as_bitbuf(obj) except NULL:
    # Borrowed pointer to the BitBuf embedded in a Python bitbuf. The checked
    # cast (<PyBitBuf?>) raises TypeError for anything that is not a bitbuf.
    return &(<PyBitBuf?> obj).bitbuf


cdef object _words_to_int(uint64_t *buf, uint32_t nwords):
    cdef object result = 0
    cdef uint32_t i
    for i in range(nwords):
        result |= (<object> buf[i]) << (64 * i)
    return result


# --------------------------------------------------------------------------
# allocate brand-new C++ BitBuf objects (stack, auto-recycled)
# --------------------------------------------------------------------------

def test_make_zeros():
    cdef BitBuf b = BitBuf.zeros(32)
    o = _to_py(&b)
    assert isinstance(o, bitbuf.bitbuf)
    assert o.width == 32
    assert int(o) == 0


def test_make_ones():
    cdef BitBuf b = BitBuf.ones(16)
    o = _to_py(&b)
    assert o.width == 16
    assert int(o) == 0xFFFF


def test_make_from_uint():
    cdef uint64_t value = 0xABCD
    cdef BitBuf b = BitBuf(<const void *> &value, 16)
    o = _to_py(&b)
    assert o.width == 16
    assert int(o) == 0xABCD
    assert o == bitbuf.bitbuf(0xABCD, 16)


def test_build_with_set():
    cdef BitBuf b = BitBuf.zeros(8)
    cdef uint32_t p
    for p in (0, 2, 4):
        b.set_bit(p, 1)
    assert int(_to_py(&b)) == 0b0001_0101  # bits 0, 2, 4


def test_append_then_pop():
    cdef BitBuf b = BitBuf.zeros(0)
    cdef uint64_t value = 0xABCD
    cdef uint64_t out = 0
    b.append_high(&value, 16)
    b.append_high(&value, 16)
    assert b.len() == 32
    b.pop_high(&out, 16)
    assert out == 0xABCD
    assert b.len() == 16


# --------------------------------------------------------------------------
# borrow the BitBuf inside an existing Python bitbuf (zero-copy)
# --------------------------------------------------------------------------

def test_borrow_set_ones_mutates_in_place():
    cdef BitBuf *p
    b = bitbuf.bitbuf.zeros(16)
    p = _as_bitbuf(b)
    p.set_ones(0, 8)
    # The very same object Python holds reflects the C++ mutation.
    assert int(b) == 0x00FF


def test_borrow_toggle():
    cdef BitBuf *p
    b = bitbuf.bitbuf.zeros(8)
    p = _as_bitbuf(b)
    p.toggle(0, 8)
    assert int(b) == 0xFF
    p.toggle(0, 4)
    assert int(b) == 0xF0


def test_borrow_get_bit_and_bits():
    cdef BitBuf *p
    cdef uint64_t low = 0
    b = bitbuf.bitbuf(0xABCD, 16)
    p = _as_bitbuf(b)
    assert p.get_bit(0) == 1  # 0xD -> ...1101, bit0 = 1
    assert p.get_bit(1) == 0
    assert p.len() == 16
    p.get_bits(0, 8, &low)
    assert low == 0xCD


def test_borrow_get_bits_wide():
    cdef BitBuf *p
    cdef uint64_t *buf
    value = (0xDEADBEEFCAFEBABE << 64) | 0x0123456789ABCDEF
    b = bitbuf.bitbuf(value, 128)
    p = _as_bitbuf(b)
    buf = <uint64_t *> malloc(2 * sizeof(uint64_t))
    if buf == NULL:
        raise MemoryError()
    try:
        p.get_bits(0, 128, buf)
        assert _words_to_int(buf, 2) == value
    finally:
        free(buf)


def test_borrow_slice():
    cdef BitBuf *src
    cdef BitBuf piece
    b = bitbuf.bitbuf(0xABCD, 16)
    src = _as_bitbuf(b)
    piece = src.slice(4, 8)
    o = _to_py(&piece)
    assert isinstance(o, bitbuf.bitbuf)
    assert o.width == 8
    assert int(o) == 0xBC  # (0xABCD >> 4) & 0xFF


def test_borrow_compare():
    cdef BitBuf *pa
    a = bitbuf.bitbuf(0xABCD, 16)
    b = bitbuf.bitbuf(0xABCD, 16)
    c = bitbuf.bitbuf(0x1234, 16)
    pa = _as_bitbuf(a)
    assert pa.compare(_as_bitbuf(b)[0]) is True
    assert pa.compare(_as_bitbuf(c)[0]) is False


def test_borrow_out_of_range_raises():
    cdef BitBuf *p
    cdef uint64_t out = 0
    b = bitbuf.bitbuf.zeros(16)
    p = _as_bitbuf(b)
    with pytest.raises(Exception):  # C++ std::exception -> Python (except +)
        p.get_bits(0, p.len() + 64, &out)


def test_as_bitbuf_type_error():
    with pytest.raises(TypeError):
        _reject_non_bitbuf(object())


cdef bint _reject_non_bitbuf(object o) except -1:
    cdef BitBuf *p = _as_bitbuf(o)  # must raise TypeError before returning
    return p != NULL


# --------------------------------------------------------------------------
# heap-owned C++ object behind a cdef class (manual new/del lifetime)
# --------------------------------------------------------------------------

cdef class BitVec:
    cdef BitBuf *_b

    def __cinit__(self, uint32_t width):
        self._b = new BitBuf()
        self._b.assign_zeros(width)

    def __dealloc__(self):
        del self._b

    def set(self, uint32_t pos):
        self._b.set_bit(pos, 1)

    def get(self, uint32_t pos):
        return self._b.get_bit(pos)

    def __len__(self):
        return self._b.len()

    def to_bitbuf(self):
        return _to_py(self._b)


def test_bitvec_heap_owned():
    v = BitVec(100)
    assert len(v) == 100
    v.set(3)
    v.set(99)
    assert v.get(3) == 1
    assert v.get(99) == 1
    assert v.get(0) == 0

    o = v.to_bitbuf()
    assert isinstance(o, bitbuf.bitbuf)
    assert o.width == 100
    assert o.get_bit(3) == 1
    assert o.get_bit(99) == 1

    del v  # exercises __dealloc__ -> del self._b (no crash / leak)
