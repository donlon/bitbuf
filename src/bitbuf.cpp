#include "bitbuf.h"

#include <algorithm>

#include <nanobind/stl/string.h>

namespace {

nb::object py_op(PyObject *(*fn)(PyObject *, PyObject *), const nb::object &a, const nb::object &b) {
    PyObject *result = fn(a.ptr(), b.ptr());
    if (!result) {
        throw nb::python_error();
    }
    return nb::steal<nb::object>(result);
}

nb::object py_unary(PyObject *(*fn)(PyObject *), const nb::object &a) {
    PyObject *result = fn(a.ptr());
    if (!result) {
        throw nb::python_error();
    }
    return nb::steal<nb::object>(result);
}

nb::int_ py_lshift(const nb::object &a, std::size_t bits) {
    return nb::cast<nb::int_>(py_op(&PyNumber_Lshift, a, nb::int_(bits)));
}

nb::int_ py_rshift(const nb::object &a, std::size_t bits) {
    return nb::cast<nb::int_>(py_op(&PyNumber_Rshift, a, nb::int_(bits)));
}

nb::int_ py_and(const nb::object &a, const nb::object &b) {
    return nb::cast<nb::int_>(py_op(&PyNumber_And, a, b));
}

nb::int_ py_or(const nb::object &a, const nb::object &b) {
    return nb::cast<nb::int_>(py_op(&PyNumber_Or, a, b));
}

nb::int_ py_xor(const nb::object &a, const nb::object &b) {
    return nb::cast<nb::int_>(py_op(&PyNumber_Xor, a, b));
}

nb::int_ py_sub(const nb::object &a, const nb::object &b) {
    return nb::cast<nb::int_>(py_op(&PyNumber_Subtract, a, b));
}

nb::int_ py_invert(const nb::object &a) {
    return nb::cast<nb::int_>(py_unary(&PyNumber_Invert, a));
}

} // namespace

BitBuf::BitBuf() : data_(nb::int_(0)), offset_(0), width_(0) {}

BitBuf::BitBuf(const nb::object &value, const nb::object &width) : data_(nb::int_(0)), offset_(0), width_(0) {
    auto [data, width_value] = sized_value(value, width);
    data_ = data;
    width_ = width_value;
    offset_ = 0;
    trim();
}

BitBuf BitBuf::from_int(const nb::object &data, const nb::object &width) {
    nb::object inferred_width = width;
    nb::int_ value = ensure_int(data);
    if (width.is_none()) {
        inferred_width = nb::int_(bit_length(value));
    }
    return BitBuf(value, inferred_width);
}

BitBuf BitBuf::from_bytes(const nb::object &data, const nb::object &width) {
    if (!is_bytes_like(data)) {
        throw nb::type_error("value must be an int or bytes-like object");
    }
    nb::object inferred_width = width;
    if (width.is_none()) {
        inferred_width = nb::int_(nb::len(data) * 8);
    }
    return BitBuf(data, inferred_width);
}

BitBuf BitBuf::zeros(const nb::object &width) {
    return BitBuf(nb::int_(0), width);
}

BitBuf BitBuf::ones(const nb::object &width) {
    std::size_t width_value = ensure_non_negative_size(width, "width");
    nb::int_ all_ones = width_value == 0 ? nb::int_(0) : py_sub(py_lshift(nb::int_(1), width_value), nb::int_(1));
    return BitBuf(all_ones, nb::int_(width_value));
}

nb::object BitBuf::eq(const nb::object &other) const {
    if (!nb::isinstance<BitBuf>(other)) {
        return nb::cast(Py_NotImplemented);
    }
    const BitBuf &rhs = nb::cast<const BitBuf &>(other);
    int eq_result = PyObject_RichCompareBool(int_py().ptr(), rhs.int_py().ptr(), Py_EQ);
    if (eq_result < 0) {
        throw nb::python_error();
    }
    return nb::bool_(width_ == rhs.width_ && eq_result == 1);
}

std::size_t BitBuf::len() const {
    return width_;
}

nb::int_ BitBuf::as_int() const {
    return int_py();
}

nb::int_ BitBuf::as_index() const {
    return int_py();
}

nb::bytes BitBuf::as_bytes() const {
    return bytes();
}

std::string BitBuf::repr() const {
    std::size_t hex_digits = std::max<std::size_t>(1, (width_ + 3) / 4);
    std::string hex_value;
    if (hex_digits <= 64) {
        hex_value = hex();
    } else {
        nb::int_ low = get_bits(nb::int_(0), nb::int_(64));
        std::size_t high_width = std::min<std::size_t>(64, width_);
        nb::int_ high = get_bits(nb::int_(width_ - high_width), nb::int_(high_width));
        std::string high_hex = to_hex_string(high).substr(2);
        std::string low_hex = to_hex_string(low).substr(2);
        if (low_hex.size() < 16) {
            low_hex.insert(0, 16 - low_hex.size(), '0');
        }
        hex_value = "0x" + high_hex + "..." + low_hex;
    }
    return "bitbuf(len=" + std::to_string(width_) + ", hex=" + hex_value + ")";
}

nb::object BitBuf::getitem(const nb::object &key) const {
    if (nb::isinstance<nb::slice>(key)) {
        auto [start, stop] = slice_bounds(key);
        return get_bits(nb::int_(start), nb::int_(stop - start));
    }
    if (nb::isinstance<nb::int_>(key)) {
        return get_bit(key);
    }
    throw nb::type_error("bit index must be an int or slice");
}

void BitBuf::setitem(const nb::object &key, const nb::object &value) {
    if (nb::isinstance<nb::slice>(key)) {
        auto [start, stop] = slice_bounds(key);
        set_bits(nb::int_(start), value, nb::int_(stop - start));
        return;
    }
    if (nb::isinstance<nb::int_>(key)) {
        set_bit(key, value);
        return;
    }
    throw nb::type_error("bit index must be an int or slice");
}

BitBuf &BitBuf::ilshift(const nb::object &bits) {
    return lshift(bits);
}

BitBuf &BitBuf::irshift(const nb::object &bits) {
    return rshift(bits);
}

BitBuf &BitBuf::assign(const nb::object &value, const nb::object &width) {
    auto [data, width_value] = sized_value(value, width);
    data_ = data;
    width_ = width_value;
    offset_ = 0;
    trim();
    return *this;
}

BitBuf &BitBuf::resize(const nb::object &width) {
    width_ = ensure_non_negative_size(width, "width");
    trim();
    return *this;
}

BitBuf &BitBuf::clear() {
    data_ = nb::int_(0);
    offset_ = 0;
    return *this;
}

nb::int_ BitBuf::get_bit(const nb::object &pos) const {
    std::size_t position = normalize_position(nb::cast<std::ptrdiff_t>(pos), false);
    return py_and(py_rshift(data_, offset_ + position), nb::int_(1));
}

nb::int_ BitBuf::get_bits(const nb::object &pos, const nb::object &width) const {
    std::ptrdiff_t position = nb::cast<std::ptrdiff_t>(pos);
    std::ptrdiff_t width_value = nb::cast<std::ptrdiff_t>(width);
    if (width_value < 0) {
        throw nb::value_error("width must be non-negative");
    }
    if (position < 0 || static_cast<std::size_t>(position + width_value) > width_) {
        throw nb::index_error("bit range out of range");
    }
    if (width_value == 0) {
        return nb::int_(0);
    }
    return py_and(py_rshift(data_, offset_ + static_cast<std::size_t>(position)), mask_for_width(static_cast<std::size_t>(width_value)));
}

nb::bytes BitBuf::get_bits_as_bytes(const nb::object &pos, const nb::object &width) const {
    nb::int_ value = get_bits(pos, width);
    std::size_t width_value = ensure_non_negative_size(width, "width");
    std::size_t byte_count = (width_value + 7) / 8;
    return nb::cast<nb::bytes>(value.attr("to_bytes")(nb::int_(byte_count), nb::str("little")));
}

nb::bytearray BitBuf::get_bits_as_bytearray(const nb::object &pos, const nb::object &width) const {
    return nb::bytearray(get_bits_as_bytes(pos, width));
}

BitBuf BitBuf::slice(const nb::object &pos, const nb::object &width) const {
    return BitBuf(get_bits(pos, width), width);
}

BitBuf &BitBuf::set_bit(const nb::object &pos, const nb::object &value) {
    std::size_t normalized = normalize_position(nb::cast<std::ptrdiff_t>(pos), false);
    set_bits(nb::int_(normalized), value, nb::int_(1));
    return *this;
}

BitBuf &BitBuf::set_bits(const nb::object &pos, const nb::object &value, const nb::object &width) {
    auto [value_data, width_value] = sized_value(value, width);
    std::ptrdiff_t pos_value = nb::cast<std::ptrdiff_t>(pos);
    if (pos_value < 0 || static_cast<std::size_t>(pos_value) + width_value > width_) {
        throw nb::index_error("bit range out of range");
    }
    if (width_value == 0) {
        return *this;
    }
    std::size_t shifted_pos = static_cast<std::size_t>(pos_value) + offset_;
    nb::int_ width_mask = mask_for_width(width_value);
    nb::int_ mask = py_lshift(width_mask, shifted_pos);
    nb::int_ value_mask = py_lshift(py_and(value_data, width_mask), shifted_pos);
    data_ = py_or(py_and(data_, py_invert(mask)), value_mask);
    return *this;
}

BitBuf &BitBuf::set_ones(const nb::object &pos, const nb::object &width) {
    std::ptrdiff_t pos_value = nb::cast<std::ptrdiff_t>(pos);
    std::ptrdiff_t width_value = nb::cast<std::ptrdiff_t>(width);
    if (width_value < 0) {
        throw nb::value_error("width must be non-negative");
    }
    if (pos_value < 0 || static_cast<std::size_t>(pos_value + width_value) > width_) {
        throw nb::index_error("bit range out of range");
    }
    if (width_value == 0) {
        return *this;
    }
    data_ = py_or(data_, py_lshift(mask_for_width(static_cast<std::size_t>(width_value)), offset_ + static_cast<std::size_t>(pos_value)));
    return *this;
}

BitBuf &BitBuf::set_zeros(const nb::object &pos, const nb::object &width) {
    std::ptrdiff_t pos_value = nb::cast<std::ptrdiff_t>(pos);
    std::ptrdiff_t width_value = nb::cast<std::ptrdiff_t>(width);
    if (width_value < 0) {
        throw nb::value_error("width must be non-negative");
    }
    if (pos_value < 0 || static_cast<std::size_t>(pos_value + width_value) > width_) {
        throw nb::index_error("bit range out of range");
    }
    if (width_value == 0) {
        return *this;
    }
    nb::int_ mask = py_lshift(mask_for_width(static_cast<std::size_t>(width_value)), offset_ + static_cast<std::size_t>(pos_value));
    data_ = py_and(data_, py_invert(mask));
    return *this;
}

BitBuf &BitBuf::toggle(const nb::object &pos, const nb::object &width) {
    std::ptrdiff_t pos_value = nb::cast<std::ptrdiff_t>(pos);
    if (width_ == 0 && pos_value == 0 && width.is_none()) {
        return *this;
    }
    if (pos_value < 0 || static_cast<std::size_t>(pos_value) >= width_) {
        throw nb::index_error("bit range out of range");
    }

    std::size_t width_value;
    if (width.is_none()) {
        width_value = width_ - static_cast<std::size_t>(pos_value);
    } else {
        std::ptrdiff_t explicit_width = nb::cast<std::ptrdiff_t>(width);
        if (explicit_width < 0) {
            throw nb::value_error("width must be non-negative");
        }
        width_value = static_cast<std::size_t>(explicit_width);
    }

    if (static_cast<std::size_t>(pos_value) + width_value > width_) {
        throw nb::index_error("bit range out of range");
    }
    if (width_value == 0 || width_ == 0) {
        return *this;
    }

    nb::int_ mask = py_lshift(mask_for_width(width_value), offset_ + static_cast<std::size_t>(pos_value));
    data_ = py_xor(data_, mask);
    return *this;
}

BitBuf &BitBuf::lshift(const nb::object &bits) {
    std::ptrdiff_t bits_value = nb::cast<std::ptrdiff_t>(bits);
    if (bits_value < 0) {
        throw nb::value_error("bits must be non-negative");
    }
    if (static_cast<std::size_t>(bits_value) >= width_) {
        data_ = nb::int_(0);
        offset_ = 0;
    } else {
        decrease_offset(static_cast<std::size_t>(bits_value));
        trim();
    }
    return *this;
}

BitBuf &BitBuf::rshift(const nb::object &bits) {
    std::ptrdiff_t bits_value = nb::cast<std::ptrdiff_t>(bits);
    if (bits_value < 0) {
        throw nb::value_error("bits must be non-negative");
    }
    if (static_cast<std::size_t>(bits_value) >= width_) {
        data_ = nb::int_(0);
        offset_ = 0;
    } else {
        increase_offset(static_cast<std::size_t>(bits_value));
        trim();
    }
    return *this;
}

BitBuf &BitBuf::append_low(const nb::object &value, const nb::object &width) {
    auto [value_data, width_value] = sized_value(value, width);
    if (width_value == 0) {
        return *this;
    }
    value_data = py_and(value_data, mask_for_width(width_value));
    decrease_offset(width_value);
    width_ += width_value;
    set_bits(nb::int_(0), value_data, nb::int_(width_value));
    trim();
    return *this;
}

BitBuf &BitBuf::append_high(const nb::object &value, const nb::object &width) {
    auto [value_data, width_value] = sized_value(value, width);
    if (width_value > 0) {
        value_data = py_and(value_data, mask_for_width(width_value));
    } else {
        value_data = nb::int_(0);
    }
    data_ = py_or(data_, py_lshift(value_data, offset_ + width_));
    width_ += width_value;
    return *this;
}

BitBuf &BitBuf::delete_low(const nb::object &width) {
    std::ptrdiff_t width_value = nb::cast<std::ptrdiff_t>(width);
    if (width_value < 0) {
        throw nb::value_error("width must be non-negative");
    }
    if (static_cast<std::size_t>(width_value) > width_) {
        throw nb::index_error("bit range out of range");
    }
    if (width_value == 0) {
        return *this;
    }
    width_ -= static_cast<std::size_t>(width_value);
    increase_offset(static_cast<std::size_t>(width_value));
    trim();
    return *this;
}

BitBuf &BitBuf::delete_high(const nb::object &width) {
    std::ptrdiff_t width_value = nb::cast<std::ptrdiff_t>(width);
    if (width_value < 0) {
        throw nb::value_error("width must be non-negative");
    }
    if (static_cast<std::size_t>(width_value) > width_) {
        throw nb::index_error("bit range out of range");
    }
    if (width_value == 0) {
        return *this;
    }
    width_ -= static_cast<std::size_t>(width_value);
    trim();
    return *this;
}

nb::int_ BitBuf::pop_low(const nb::object &width) {
    std::ptrdiff_t width_value = nb::cast<std::ptrdiff_t>(width);
    if (width_value < 0) {
        throw nb::value_error("width must be non-negative");
    }
    if (static_cast<std::size_t>(width_value) > width_) {
        throw nb::index_error("bit range out of range");
    }
    if (width_value == 0) {
        return nb::int_(0);
    }
    nb::int_ value = get_bits(nb::int_(0), nb::int_(width_value));
    delete_low(nb::int_(width_value));
    return value;
}

nb::int_ BitBuf::pop_high(const nb::object &width) {
    std::ptrdiff_t width_value = nb::cast<std::ptrdiff_t>(width);
    if (width_value < 0) {
        throw nb::value_error("width must be non-negative");
    }
    if (static_cast<std::size_t>(width_value) > width_) {
        throw nb::index_error("bit range out of range");
    }
    if (width_value == 0) {
        return nb::int_(0);
    }
    std::size_t pos = width_ - static_cast<std::size_t>(width_value);
    nb::int_ value = get_bits(nb::int_(pos), nb::int_(width_value));
    delete_high(nb::int_(width_value));
    return value;
}

nb::bytearray BitBuf::as_bytearray() const {
    return nb::bytearray(bytes());
}

nb::bytes BitBuf::bytes() const {
    return nb::cast<nb::bytes>(int_py().attr("to_bytes")(nb::int_(nbytes()), nb::str("little")));
}

std::string BitBuf::hex() const {
    return to_hex_string(int_py());
}

nb::int_ BitBuf::int_value() const {
    return int_py();
}

std::size_t BitBuf::width() const {
    return width_;
}

std::size_t BitBuf::nbytes() const {
    return width_ == 0 ? 0 : (width_ + 7) / 8;
}

std::size_t BitBuf::offset() const {
    return offset_;
}

bool BitBuf::is_bytes_like(const nb::handle &value) {
    return PyBytes_Check(value.ptr()) || PyByteArray_Check(value.ptr()) || PyMemoryView_Check(value.ptr());
}

nb::int_ BitBuf::ensure_int(const nb::object &value) {
    if (nb::isinstance<nb::int_>(value)) {
        return nb::cast<nb::int_>(value);
    }
    if (is_bytes_like(value)) {
        nb::object builtins_int = nb::module_::import_("builtins").attr("int");
        return nb::cast<nb::int_>(builtins_int.attr("from_bytes")(value, nb::str("little")));
    }
    throw nb::type_error("value must be an int or bytes-like object");
}

std::size_t BitBuf::ensure_non_negative_size(const nb::object &value, const char *name) {
    std::ptrdiff_t result = nb::cast<std::ptrdiff_t>(value);
    if (result < 0) {
        throw nb::value_error((std::string(name) + " must be non-negative").c_str());
    }
    return static_cast<std::size_t>(result);
}

nb::int_ BitBuf::mask_for_width(std::size_t width) {
    if (width == 0) {
        return nb::int_(0);
    }
    return py_sub(py_lshift(nb::int_(1), width), nb::int_(1));
}

std::size_t BitBuf::bit_length(const nb::int_ &value) {
    return nb::cast<std::size_t>(value.attr("bit_length")());
}

std::string BitBuf::to_hex_string(const nb::int_ &value) {
    return nb::cast<std::string>(nb::module_::import_("builtins").attr("hex")(value));
}

std::pair<nb::int_, std::size_t> BitBuf::sized_value(const nb::object &value, const nb::object &width) const {
    if (nb::isinstance<BitBuf>(value)) {
        const BitBuf &other = nb::cast<const BitBuf &>(value);
        return {other.int_py(), other.len()};
    }

    nb::int_ data = ensure_int(value);
    std::size_t width_value;

    if (is_bytes_like(value)) {
        if (width.is_none()) {
            width_value = static_cast<std::size_t>(nb::len(value)) * 8;
        } else {
            width_value = ensure_non_negative_size(width, "width");
        }
    } else {
        if (width.is_none()) {
            width_value = bit_length(data);
        } else {
            width_value = ensure_non_negative_size(width, "width");
        }
    }

    return {data, width_value};
}

std::size_t BitBuf::normalize_position(std::ptrdiff_t pos, bool allow_end) const {
    if (pos < 0) {
        pos += static_cast<std::ptrdiff_t>(width_);
    }
    std::ptrdiff_t upper = allow_end ? static_cast<std::ptrdiff_t>(width_) : static_cast<std::ptrdiff_t>(width_) - 1;
    if (pos < 0 || pos > upper) {
        throw nb::index_error("bit pos out of range");
    }
    return static_cast<std::size_t>(pos);
}

std::pair<std::size_t, std::size_t> BitBuf::slice_bounds(const nb::object &slice_obj) const {
    nb::object step = slice_obj.attr("step");
    if (!step.is_none()) {
        throw nb::type_error("slice step is not supported");
    }
    nb::object start_obj = slice_obj.attr("start");
    nb::object stop_obj = slice_obj.attr("stop");
    if (start_obj.is_none() || stop_obj.is_none()) {
        throw nb::type_error("slice start and stop are required");
    }
    std::size_t start = normalize_position(nb::cast<std::ptrdiff_t>(start_obj), true);
    std::size_t stop = normalize_position(nb::cast<std::ptrdiff_t>(stop_obj), true);
    if (stop < start) {
        throw nb::value_error("slice stop must be greater than or equal to start");
    }
    return {start, stop};
}

nb::int_ BitBuf::int_py() const {
    return py_and(py_rshift(data_, offset_), mask_for_width(width_));
}

void BitBuf::trim() {
    if (width_ == 0) {
        data_ = nb::int_(0);
        offset_ = 0;
        return;
    }
    data_ = py_and(data_, py_lshift(mask_for_width(width_), offset_));
}

void BitBuf::increase_offset(std::size_t bits) {
    offset_ += bits;
    if (offset_ >= 32) {
        std::size_t words = offset_ / 32;
        data_ = py_rshift(data_, words * 32);
        offset_ %= 32;
    }
}

void BitBuf::decrease_offset(std::size_t bits) {
    if (bits <= offset_) {
        offset_ -= bits;
        return;
    }
    std::size_t words = (bits - offset_ + 31) / 32;
    data_ = py_lshift(data_, words * 32);
    offset_ += words * 32 - bits;
}
