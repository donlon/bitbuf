#include "bitbuf.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>

namespace nb = nanobind;

NB_MODULE(_bitbuf, m) {
    m.doc() = "nanobind C++ implementation for bitbuf";

    nb::class_<BitBuf> cls(m, "bitbuf");

    cls.def(nb::init<>())
        .def(
            nb::init<const nb::object &, const nb::object &>(),
            nb::arg("value") = nb::int_(0),
            nb::arg("width") = nb::none())
        .def_static("from_int", &BitBuf::from_int, nb::arg("data"), nb::arg("width") = nb::none())
        .def_static("from_bytes", &BitBuf::from_bytes, nb::arg("data"), nb::arg("width") = nb::none())
        .def_static("zeros", &BitBuf::zeros, nb::arg("width"))
        .def_static("ones", &BitBuf::ones, nb::arg("width"))
        .def("__eq__", &BitBuf::eq)
        .def("__len__", &BitBuf::len)
        .def("__int__", &BitBuf::as_int)
        .def("__index__", &BitBuf::as_index)
        .def("__bytes__", &BitBuf::as_bytes)
        .def("__repr__", &BitBuf::repr)
        .def("__getitem__", &BitBuf::getitem)
        .def("__setitem__", &BitBuf::setitem)
        .def("__ilshift__", &BitBuf::ilshift, nb::rv_policy::reference_internal)
        .def("__irshift__", &BitBuf::irshift, nb::rv_policy::reference_internal)
        .def("assign", &BitBuf::assign, nb::arg("value") = nb::int_(0), nb::arg("width") = nb::none(), nb::rv_policy::reference_internal)
        .def("resize", &BitBuf::resize, nb::arg("width"), nb::rv_policy::reference_internal)
        .def("clear", &BitBuf::clear, nb::rv_policy::reference_internal)
        .def("get_bit", &BitBuf::get_bit, nb::arg("pos"))
        .def("get_bits", &BitBuf::get_bits, nb::arg("pos"), nb::arg("width"))
        .def("get_bits_as_bytes", &BitBuf::get_bits_as_bytes, nb::arg("pos"), nb::arg("width"))
        .def("get_bits_as_bytearray", &BitBuf::get_bits_as_bytearray, nb::arg("pos"), nb::arg("width"))
        .def("slice", &BitBuf::slice, nb::arg("pos"), nb::arg("width"))
        .def("set_bit", &BitBuf::set_bit, nb::arg("pos"), nb::arg("value") = nb::int_(1), nb::rv_policy::reference_internal)
        .def("set_bits", &BitBuf::set_bits, nb::arg("pos"), nb::arg("value") = nb::int_(0), nb::arg("width") = nb::none(), nb::rv_policy::reference_internal)
        .def("set_ones", &BitBuf::set_ones, nb::arg("pos"), nb::arg("width"), nb::rv_policy::reference_internal)
        .def("set_zeros", &BitBuf::set_zeros, nb::arg("pos"), nb::arg("width"), nb::rv_policy::reference_internal)
        .def("toggle", &BitBuf::toggle, nb::arg("pos") = nb::int_(0), nb::arg("width") = nb::none(), nb::rv_policy::reference_internal)
        .def("lshift", &BitBuf::lshift, nb::arg("bits"), nb::rv_policy::reference_internal)
        .def("rshift", &BitBuf::rshift, nb::arg("bits"), nb::rv_policy::reference_internal)
        .def("append_low", &BitBuf::append_low, nb::arg("value") = nb::int_(0), nb::arg("width") = nb::none(), nb::rv_policy::reference_internal)
        .def("append_high", &BitBuf::append_high, nb::arg("value") = nb::int_(0), nb::arg("width") = nb::none(), nb::rv_policy::reference_internal)
        .def("delete_low", &BitBuf::delete_low, nb::arg("width"), nb::rv_policy::reference_internal)
        .def("delete_high", &BitBuf::delete_high, nb::arg("width"), nb::rv_policy::reference_internal)
        .def("pop_low", &BitBuf::pop_low, nb::arg("width"))
        .def("pop_high", &BitBuf::pop_high, nb::arg("width"))
        .def("bytearray", &BitBuf::as_bytearray)
        .def("bytes", &BitBuf::bytes)
        .def("hex", &BitBuf::hex)
        .def("int", &BitBuf::int_value)
        .def_prop_ro("width", &BitBuf::width)
        .def_prop_ro("nbytes", &BitBuf::nbytes)
        .def_prop_ro("_offset", &BitBuf::offset);
}
