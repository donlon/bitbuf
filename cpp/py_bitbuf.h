#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

#include <Python.h>

#include "bitbuf.h"

// clang-format off
typedef struct {
    PyObject_HEAD
    BitBuf bitbuf;
} PyBitBufObject;
// clang-format on

// static_assert(sizeof(PyBitBufObject) <= 64, "Size of BitBuf should less than size of cache line");

extern PyTypeObject PyBitBufType;

#define PyBitBuf_Check(op) PyObject_TypeCheck((op), &PyBitBufType)
#define PyBitBuf_CAST(op) (reinterpret_cast<PyBitBufObject *>(op))

PyObject *PyBitBuf_new(PyTypeObject *type, PyObject *args, PyObject *kwds);
int PyBitBuf_init(PyBitBufObject *self, PyObject *args, PyObject *kwds);
void PyBitBuf_dealloc(PyBitBufObject *self);

PyObject *PyBitBuf_from_int(PyObject *cls, PyObject *args, PyObject *kwargs);
PyObject *PyBitBuf_from_bytes(PyObject *cls, PyObject *args, PyObject *kwargs);
PyObject *PyBitBuf_zeros(PyObject *cls, PyObject *width);
PyObject *PyBitBuf_ones(PyObject *cls, PyObject *width);

PyObject *PyBitBuf_richcompare(PyObject *self, PyObject *other, int op);
Py_ssize_t PyBitBuf_len(PyObject *self);
PyObject *PyBitBuf_as_int(PyObject *self, PyObject *Py_UNUSED(ignored));
PyObject *PyBitBuf_as_index(PyObject *self);
PyObject *PyBitBuf_as_bytes(PyObject *self, PyObject *Py_UNUSED(ignored));
PyObject *PyBitBuf_repr(PyObject *self);
PyObject *PyBitBuf_getitem(PyObject *self, PyObject *key);
int PyBitBuf_setitem(PyObject *self, PyObject *key, PyObject *value);

PyObject *PyBitBuf_ilshift(PyObject *self, PyObject *arg);
PyObject *PyBitBuf_irshift(PyObject *self, PyObject *arg);

PyObject *PyBitBuf_assign(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *PyBitBuf_resize(PyObject *self, PyObject *width_);
PyObject *PyBitBuf_clear(PyObject *self, PyObject *Py_UNUSED(ignored));
PyObject *PyBitBuf_get_bit(PyObject *self, PyObject *pos_);
PyObject *PyBitBuf_get_bits(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *PyBitBuf_get_bits_as_bytes(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *PyBitBuf_get_bits_as_bytearray(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *PyBitBuf_slice(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *PyBitBuf_set_bit(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *PyBitBuf_set_bits(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *PyBitBuf_set_ones(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *PyBitBuf_set_zeros(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *PyBitBuf_toggle(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *PyBitBuf_lshift(PyObject *self, PyObject *bits_);
PyObject *PyBitBuf_rshift(PyObject *self, PyObject *bits_);
PyObject *PyBitBuf_append_low(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *PyBitBuf_append_high(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *PyBitBuf_delete_low(PyObject *self, PyObject *width_);
PyObject *PyBitBuf_delete_high(PyObject *self, PyObject *width_);
PyObject *PyBitBuf_pop_low(PyObject *self, PyObject *width_);
PyObject *PyBitBuf_pop_high(PyObject *self, PyObject *width_);
PyObject *PyBitBuf_bytearray(PyObject *self, PyObject *Py_UNUSED(ignored));
PyObject *PyBitBuf_bytes_method(PyObject *self, PyObject *Py_UNUSED(ignored));
PyObject *PyBitBuf_hex(PyObject *self, PyObject *Py_UNUSED(ignored));
PyObject *PyBitBuf_int_method(PyObject *self, PyObject *Py_UNUSED(ignored));

PyObject *PyBitBuf_get_width(PyObject *self, void *closure);
PyObject *PyBitBuf_get_nbytes(PyObject *self, void *closure);
PyObject *PyBitBuf_get_offset(PyObject *self, void *closure);
