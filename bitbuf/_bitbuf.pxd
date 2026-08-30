# SPDX-License-Identifier: MIT
#
# Declarations for the bitbuf._bitbuf extension: the C++ BitBuf class and the
# ``bitbuf`` extension type. Consumers should cimport these via the package
# (``from bitbuf cimport BitBuf, bitbuf``), which re-exports them.

from libc.stdint cimport uint8_t, uint32_t, uint64_t


cdef extern from "bitbuf.h":
    cdef cppclass BitBuf:
        ctypedef uint64_t data_t

        BitBuf() except +
        BitBuf(const void *buffer, uint32_t length) except +

        @staticmethod
        BitBuf ones(uint32_t length) except +
        @staticmethod
        BitBuf zeros(uint32_t length) except +

        BitBuf &assign(const void *buffer, uint32_t length)
        BitBuf &assign_zeros(uint32_t length)
        BitBuf &assign_ones(uint32_t length)
        bint compare(BitBuf &other)

        uint32_t len() const
        uint32_t nbytes() const
        uint32_t get_offset() const

        BitBuf &resize(uint32_t length)
        BitBuf &clear()

        int get_bit(uint32_t pos) const
        void get_bits(uint32_t pos, uint32_t length, data_t *dst_buf) except +
        BitBuf slice(uint32_t pos, uint32_t length) const

        BitBuf &set_bit(uint32_t pos, uint32_t value)
        BitBuf &set_bits(uint32_t pos, const data_t *src_buffer, uint32_t length)
        BitBuf &set_ones(uint32_t pos, uint32_t length) except +
        BitBuf &set_zeros(uint32_t pos, uint32_t length) except +
        BitBuf &toggle(uint32_t pos, uint32_t length) except +

        BitBuf &lshift(uint32_t bits)
        BitBuf &rshift(uint32_t bits)
        BitBuf &append_low(const data_t *value, uint32_t length)
        BitBuf &append_high(const data_t *value, uint32_t length)
        BitBuf &delete_low(uint32_t length)
        BitBuf &delete_high(uint32_t length)
        void pop_low(data_t *dst_buffer, uint32_t length)
        void pop_high(data_t *dst_buffer, uint32_t length)

        uint8_t *normalize_buffer_8b()


cdef class bitbuf:
    cdef BitBuf bitbuf

    cpdef void assign(self, object value=*, int length=*)
    cpdef void resize(self, int length)
    cpdef void clear(self)
    cpdef bitbuf clone(self)
    cpdef int get_bit(self, int pos)
    cpdef object get_bits(self, int pos, int length)
    cpdef bytes get_bits_as_bytes(self, int pos, int length)
    cpdef bytearray get_bits_as_bytearray(self, int pos, int length)
    cpdef bitbuf slice(self, int pos, int length)
    cpdef void clear_bit(self, int pos)
    cpdef void set_bit(self, int pos, int value=*)
    cpdef void set_bits(self, int pos, object value, int length=*)
    cpdef void set_ones(self, int pos, int length)
    cpdef void set_zeros(self, int pos, int length)
    cpdef void toggle(self, int pos=*, int length=*)
    cpdef void lshift(self, int bits)
    cpdef void rshift(self, int bits)
    cpdef void append_low(self, object value, int length=*)
    cpdef void append_high(self, object value, int length=*)
    cpdef void delete_low(self, int length)
    cpdef void delete_high(self, int length)
    cpdef object pop_low(self, int length)
    cpdef object pop_high(self, int length)
    cpdef bytearray bytearray(self)
    cpdef bytes bytes(self)
    cpdef str hex(self)
    cpdef object int(self)


cdef bitbuf create_zeros(int length)

cdef bitbuf create_ones(int length)

cdef bitbuf create_random(int length)

cdef bitbuf from_buffer(const void *buffer, size_t pos, size_t length)

cdef int ABI_VERSION
