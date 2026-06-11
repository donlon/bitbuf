// SPDX-License-Identifier: MIT

#include "py_bitbuf.h"

#define PYFUNC(function) reinterpret_cast<PyCFunction>(reinterpret_cast<void *>(function))

// #define METH_CLASS_VA (METH_VARARGS | METH_KEYWORDS | METH_CLASS)

static PyMethodDef PyBitBuf_methods[] = {
        /* Static methods */
        {"zeros",                 PYFUNC(PyBitBuf_zeros),                 METH_CLASS | METH_O,          nullptr},
        {"ones",                  PYFUNC(PyBitBuf_ones),                  METH_CLASS | METH_O,          nullptr},
        /* Builtin methods */
        {"__int__",               PYFUNC(PyBitBuf_as_int),                METH_NOARGS,                  nullptr},
        {"__index__",             PYFUNC(PyBitBuf_as_int),                METH_NOARGS,                  nullptr},
        {"__bytes__",             PYFUNC(PyBitBuf_as_bytes),              METH_NOARGS,                  nullptr},
        {"__getstate__",          PYFUNC(PyBitBuf_getstate),              METH_NOARGS,                  nullptr},
        {"__setstate__",          PYFUNC(PyBitBuf_setstate),              METH_O,                       nullptr},
        /* Class methods */
        {"assign",                PYFUNC(PyBitBuf_assign),                METH_VARARGS | METH_KEYWORDS, nullptr},
        {"resize",                PYFUNC(PyBitBuf_resize),                METH_O,                       nullptr},
        {"clear",                 PYFUNC(PyBitBuf_clear),                 METH_NOARGS,                  "Clear"},
        {"clone",                 PYFUNC(PyBitBuf_clone),                 METH_NOARGS,                  nullptr},
        {"get_bit",               PYFUNC(PyBitBuf_get_bit),               METH_O,                       nullptr},
        {"get_bits",              PYFUNC(PyBitBuf_get_bits),              METH_VARARGS | METH_KEYWORDS, nullptr},
        {"get_bits_as_bytes",     PYFUNC(PyBitBuf_get_bits_as_bytes),     METH_VARARGS | METH_KEYWORDS, nullptr},
        {"get_bits_as_bytearray", PYFUNC(PyBitBuf_get_bits_as_bytearray), METH_VARARGS | METH_KEYWORDS, nullptr},
        {"slice",                 PYFUNC(PyBitBuf_slice),                 METH_VARARGS | METH_KEYWORDS, nullptr},
        {"clear_bit",             PYFUNC(PyBitBuf_clear_bit),             METH_O,                       nullptr},
        {"set_bit",               PYFUNC(PyBitBuf_set_bit),               METH_VARARGS | METH_KEYWORDS, nullptr},
        {"set_bits",              PYFUNC(PyBitBuf_set_bits),              METH_VARARGS | METH_KEYWORDS, nullptr},
        {"set_ones",              PYFUNC(PyBitBuf_set_ones),              METH_VARARGS | METH_KEYWORDS, nullptr},
        {"set_zeros",             PYFUNC(PyBitBuf_set_zeros),             METH_VARARGS | METH_KEYWORDS, nullptr},
        {"toggle",                PYFUNC(PyBitBuf_toggle),                METH_VARARGS | METH_KEYWORDS, nullptr},
        {"lshift",                PYFUNC(PyBitBuf_lshift),                METH_O,                       nullptr},
        {"rshift",                PYFUNC(PyBitBuf_rshift),                METH_O,                       nullptr},
        {"append_low",            PYFUNC(PyBitBuf_append_low),            METH_VARARGS | METH_KEYWORDS, nullptr},
        {"append_high",           PYFUNC(PyBitBuf_append_high),           METH_VARARGS | METH_KEYWORDS, nullptr},
        {"delete_low",            PYFUNC(PyBitBuf_delete_low),            METH_O,                       nullptr},
        {"delete_high",           PYFUNC(PyBitBuf_delete_high),           METH_O,                       nullptr},
        {"pop_low",               PYFUNC(PyBitBuf_pop_low),               METH_O,                       nullptr},
        {"pop_high",              PYFUNC(PyBitBuf_pop_high),              METH_O,                       nullptr},
        {"bytearray",             PYFUNC(PyBitBuf_bytearray),             METH_NOARGS,                  nullptr},
        {"bytes",                 PYFUNC(PyBitBuf_bytes_method),          METH_NOARGS,                  nullptr},
        {"hex",                   PYFUNC(PyBitBuf_hex),                   METH_NOARGS,                  nullptr},
        {"int",                   PYFUNC(PyBitBuf_int_method),            METH_NOARGS,                  nullptr},
        {nullptr,                 nullptr,                                0,                            nullptr},
};

static PyGetSetDef PyBitBuf_getset[] = {
        {"width",  PyBitBuf_get_width,  nullptr, nullptr, nullptr},
        {"nbytes", PyBitBuf_get_nbytes, nullptr, nullptr, nullptr},
        {"offset", PyBitBuf_get_offset, nullptr, nullptr, nullptr},
        {nullptr,  nullptr,             nullptr, nullptr, nullptr},
};

static PyNumberMethods PyBitBuf_number_methods;
static PySequenceMethods PyBitBuf_sequence_methods;
static PyMappingMethods PyBitBuf_mapping_methods;

// clang-format off
PyTypeObject PyBitBufType = {
        PyVarObject_HEAD_INIT(nullptr, 0)
};

static PyModuleDef bitbuf_module = {
        PyModuleDef_HEAD_INIT,
        "_bitbuf",
        "CPython C-API implementation for bitbuf",
        -1,
        nullptr,
};
// clang-format on

PyMODINIT_FUNC PyInit__bitbuf(void) { // NOLINT
    PyBitBuf_number_methods = {};
    PyBitBuf_number_methods.nb_int = PyBitBuf_as_index;
    PyBitBuf_number_methods.nb_inplace_add = PyBitBuf_iadd;
    PyBitBuf_number_methods.nb_inplace_lshift = PyBitBuf_ilshift;
    PyBitBuf_number_methods.nb_inplace_rshift = PyBitBuf_irshift;
    PyBitBuf_number_methods.nb_index = PyBitBuf_as_index;

    PyBitBuf_sequence_methods = {};
    PyBitBuf_sequence_methods.sq_length = PyBitBuf_len;

    PyBitBuf_mapping_methods = {};
    PyBitBuf_mapping_methods.mp_length = PyBitBuf_len;
    PyBitBuf_mapping_methods.mp_subscript = PyBitBuf_getitem;
    PyBitBuf_mapping_methods.mp_ass_subscript = PyBitBuf_setitem;

    PyBitBufType.tp_name = "bitbuf.bitbuf";
    PyBitBufType.tp_basicsize = sizeof(PyBitBufObject);
    PyBitBufType.tp_itemsize = 0;
    PyBitBufType.tp_dealloc = (destructor) PyBitBuf_dealloc;
    PyBitBufType.tp_repr = PyBitBuf_repr;
    PyBitBufType.tp_as_number = &PyBitBuf_number_methods;
    PyBitBufType.tp_as_sequence = &PyBitBuf_sequence_methods;
    PyBitBufType.tp_as_mapping = &PyBitBuf_mapping_methods;
    PyBitBufType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
    PyBitBufType.tp_doc = "CPython C-API implementation for bitbuf";
    PyBitBufType.tp_richcompare = PyBitBuf_richcompare;
    PyBitBufType.tp_methods = PyBitBuf_methods;
    PyBitBufType.tp_getset = PyBitBuf_getset;
    PyBitBufType.tp_init = (initproc) PyBitBuf_init;
    PyBitBufType.tp_new = PyBitBuf_new;

    if (PyType_Ready(&PyBitBufType) < 0) {
        return nullptr;
    }

    PyObject *m = PyModule_Create(&bitbuf_module);
    if (m == nullptr) {
        return nullptr;
    }

    Py_INCREF(&PyBitBufType);
    if (PyModule_AddObject(m, "bitbuf", reinterpret_cast<PyObject *>(&PyBitBufType)) < 0) {
        Py_DECREF(&PyBitBufType);
        Py_DECREF(m);
        return nullptr;
    }

    return m;
}
