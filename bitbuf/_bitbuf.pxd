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
        BitBuf(const void *buffer, uint32_t width) except +

        @staticmethod
        BitBuf ones(uint32_t size) except +
        @staticmethod
        BitBuf zeros(uint32_t size) except +

        BitBuf &assign(const void *buffer, uint32_t width)
        BitBuf &assign_zeros(uint32_t width)
        BitBuf &assign_ones(uint32_t width)
        bint compare(BitBuf &other)

        uint32_t len() const
        uint32_t width() const
        uint32_t nbytes() const
        uint32_t get_offset() const

        BitBuf &resize(uint32_t width)
        BitBuf &clear()

        int get_bit(uint32_t pos) const
        void get_bits(uint32_t pos, uint32_t width, data_t *dst_buf) except +
        BitBuf slice(uint32_t pos, uint32_t width) const

        BitBuf &set_bit(uint32_t pos, uint32_t value)
        BitBuf &set_bits(uint32_t pos, const data_t *src_buffer, uint32_t width)
        BitBuf &set_ones(uint32_t pos, uint32_t width) except +
        BitBuf &set_zeros(uint32_t pos, uint32_t width) except +
        BitBuf &toggle(uint32_t pos, uint32_t width) except +

        BitBuf &lshift(uint32_t bits)
        BitBuf &rshift(uint32_t bits)
        BitBuf &append_low(const data_t *value, uint32_t width)
        BitBuf &append_high(const data_t *value, uint32_t width)
        BitBuf &delete_low(uint32_t width)
        BitBuf &delete_high(uint32_t width)
        void pop_low(data_t *dst_buffer, uint32_t width)
        void pop_high(data_t *dst_buffer, uint32_t width)

        uint8_t *normalize_buffer_8b()


cdef class bitbuf:
    cdef BitBuf bitbuf

    cpdef bitbuf assign(self, object value=*, int width=*)
    cpdef bitbuf resize(self, int width)
    cpdef bitbuf clear(self)
    cpdef bitbuf clone(self)
    cpdef int get_bit(self, int pos)
    cpdef object get_bits(self, int pos, int width)
    cpdef bytes get_bits_as_bytes(self, int pos, int width)
    cpdef bytearray get_bits_as_bytearray(self, int pos, int width)
    cpdef object slice(self, int pos, int width)
    cpdef bitbuf clear_bit(self, int pos)
    cpdef bitbuf set_bit(self, int pos, int value=*)
    cpdef bitbuf set_bits(self, int pos, object value, int width=*)
    cpdef bitbuf set_ones(self, int pos, int width)
    cpdef bitbuf set_zeros(self, int pos, int width)
    cpdef bitbuf toggle(self, int pos=*, int width=*)
    cpdef bitbuf lshift(self, int bits)
    cpdef bitbuf rshift(self, int bits)
    cpdef bitbuf append_low(self, object value, int width=*)
    cpdef bitbuf append_high(self, object value, int width=*)
    cpdef bitbuf delete_low(self, int width)
    cpdef bitbuf delete_high(self, int width)
    cpdef object pop_low(self, int width)
    cpdef object pop_high(self, int width)
    cpdef bytearray bytearray(self)
    cpdef bytes bytes(self)
    cpdef str hex(self)
    cpdef object int(self)


cdef bitbuf create_zeros(int width)

cdef bitbuf create_ones(int width)

cdef bitbuf from_buffer(const void *buffer, size_t offset, size_t size)
