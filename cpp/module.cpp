#include "py_bitbuf.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>

namespace nb = nanobind;

NB_MODULE(_bitbuf, m) {
    m.doc() = "nanobind C++ implementation for bitbuf";

    nb::class_<PyBitBuf> cls(m, "bitbuf");

    cls.def(nb::init<>())
        .def(
            nb::init<const nb::object &, const nb::object &>(),
            nb::arg("value") = nb::int_(0),
            nb::arg("width") = nb::none())
        .def_static("from_int", &PyBitBuf::from_int, nb::arg("data"), nb::arg("width") = nb::none())
        .def_static("from_bytes", &PyBitBuf::from_bytes, nb::arg("data"), nb::arg("width") = nb::none())
        .def_static("zeros", &PyBitBuf::zeros, nb::arg("width"))
        .def_static("ones", &PyBitBuf::ones, nb::arg("width"))
        .def("__eq__", &PyBitBuf::eq)
        .def("__len__", &PyBitBuf::len)
        .def("__int__", &PyBitBuf::as_int)
        .def("__index__", &PyBitBuf::as_index)
        .def("__bytes__", &PyBitBuf::as_bytes)
        .def("__repr__", &PyBitBuf::repr)
        .def("__getitem__", &PyBitBuf::getitem)
        .def("__setitem__", &PyBitBuf::setitem)
        .def("__ilshift__", &PyBitBuf::ilshift, nb::rv_policy::reference_internal)
        .def("__irshift__", &PyBitBuf::irshift, nb::rv_policy::reference_internal)
        .def("assign", &PyBitBuf::assign, nb::arg("value") = nb::int_(0), nb::arg("width") = nb::none(), nb::rv_policy::reference_internal)
        .def("resize", &PyBitBuf::resize, nb::arg("width"), nb::rv_policy::reference_internal)
        .def("clear", &PyBitBuf::clear, nb::rv_policy::reference_internal)
        .def("get_bit", &PyBitBuf::get_bit, nb::arg("pos"))
        .def("get_bits", &PyBitBuf::get_bits, nb::arg("pos"), nb::arg("width"))
        .def("get_bits_as_bytes", &PyBitBuf::get_bits_as_bytes, nb::arg("pos"), nb::arg("width"))
        .def("get_bits_as_bytearray", &PyBitBuf::get_bits_as_bytearray, nb::arg("pos"), nb::arg("width"))
        .def("slice", &PyBitBuf::slice, nb::arg("pos"), nb::arg("width"))
        .def("set_bit", &PyBitBuf::set_bit, nb::arg("pos"), nb::arg("value") = nb::int_(1), nb::rv_policy::reference_internal)
        .def("set_bits", &PyBitBuf::set_bits, nb::arg("pos"), nb::arg("value") = nb::int_(0), nb::arg("width") = nb::none(), nb::rv_policy::reference_internal)
        .def("set_ones", &PyBitBuf::set_ones, nb::arg("pos"), nb::arg("width"), nb::rv_policy::reference_internal)
        .def("set_zeros", &PyBitBuf::set_zeros, nb::arg("pos"), nb::arg("width"), nb::rv_policy::reference_internal)
        .def("toggle", &PyBitBuf::toggle, nb::arg("pos") = nb::int_(0), nb::arg("width") = nb::none(), nb::rv_policy::reference_internal)
        .def("lshift", &PyBitBuf::lshift, nb::arg("bits"), nb::rv_policy::reference_internal)
        .def("rshift", &PyBitBuf::rshift, nb::arg("bits"), nb::rv_policy::reference_internal)
        .def("append_low", &PyBitBuf::append_low, nb::arg("value") = nb::int_(0), nb::arg("width") = nb::none(), nb::rv_policy::reference_internal)
        .def("append_high", &PyBitBuf::append_high, nb::arg("value") = nb::int_(0), nb::arg("width") = nb::none(), nb::rv_policy::reference_internal)
        .def("delete_low", &PyBitBuf::delete_low, nb::arg("width"), nb::rv_policy::reference_internal)
        .def("delete_high", &PyBitBuf::delete_high, nb::arg("width"), nb::rv_policy::reference_internal)
        .def("pop_low", &PyBitBuf::pop_low, nb::arg("width"))
        .def("pop_high", &PyBitBuf::pop_high, nb::arg("width"))
        .def("bytearray", &PyBitBuf::as_bytearray)
        .def("bytes", &PyBitBuf::bytes)
        .def("hex", &PyBitBuf::hex)
        .def("int", &PyBitBuf::int_value)
        .def_prop_ro("width", &PyBitBuf::width)
        .def_prop_ro("nbytes", &PyBitBuf::nbytes)
        .def_prop_ro("_offset", &PyBitBuf::get_offset);
}
