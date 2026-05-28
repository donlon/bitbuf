#include "py_bitbuf.h"
#include "nanobind/ndarray.h"

#include <memory>
#include <nanobind/stl/string.h>

static std::string to_hex_string(const uint8_t *ptr, size_t size) {
    static const char *hex_table = "0123456789abcdef";
    if (size == 0) return "0";
    std::string str{"0x"};
    str.reserve(2 * size + 4);
    auto i = size - 1;
    do {
        uint8_t byte = ptr[i];
        str += hex_table[byte >> 4];
        str += hex_table[byte & 15];
    } while (i--);
    return str;
}

struct ExtractedBuffer {
    uint32_t size = 0;
    const BitBuf::data_t *buffer = nullptr;
    bool is_owned = false;

    explicit ExtractedBuffer(
            uint32_t buffer_size,
            int64_t actual_size,
            const BitBuf::data_t *buffer,
            bool is_owned = false
    ) : buffer(buffer), is_owned(is_owned) {
        if (actual_size >= 0) {
            if (actual_size > buffer_size) throw nb::value_error("width is bigger than actual data size");
            this->size = actual_size;
        } else {
            this->size = buffer_size;
        }
    }

    ExtractedBuffer(const ExtractedBuffer &other) = delete;

    ExtractedBuffer &operator=(const ExtractedBuffer &other) = delete;

    ExtractedBuffer &operator=(ExtractedBuffer &&other) = delete;

    ExtractedBuffer(ExtractedBuffer &&other) noexcept
            : size(other.size), buffer(other.buffer), is_owned(other.is_owned) {
        other.size = 0;
        other.buffer = nullptr;
        other.is_owned = false;
    }

    ~ExtractedBuffer() {
        if (is_owned) delete[] buffer;
        buffer = nullptr;
        is_owned = false;
    }

    static ExtractedBuffer extract(const nb::object &value_, const nb::object &width_) {
        PyObject *ptr = value_.ptr();
        if (PyLong_Check(ptr) && width_.is_none()) {
            throw nb::value_error("width is not specified for int data");
        }
        int64_t width;
        if (width_.is_none()) {
            auto w = nb::cast<int>(width_);
            if (w < 0) throw nb::value_error("invalid argument 'width'");
            width = w;
        } else {
            width = -1;
        }

        if (nb::isinstance<PyBitBuf>(value_)) {
            auto &other = nb::cast<PyBitBuf &>(value_);
            uint8_t *buf = other.bitbuf.normalize_buffer_8b();
            // TODO: support zero padding
            return ExtractedBuffer{
                    other.bitbuf.len(),
                    width,
                    (BitBuf::data_t *) buf,
            };
        }
        if (PyLong_Check(ptr)) {
            uint32_t size;
            PyLong_AsNativeBytes(ptr, &size, 0, Py_ASNATIVEBYTES_LITTLE_ENDIAN);
            if (size < width) size = width;
            auto buf = new uint64_t[(size + 7) / 8];
            PyLong_AsNativeBytes(ptr, buf, size, Py_ASNATIVEBYTES_LITTLE_ENDIAN);
            return ExtractedBuffer{
                    static_cast<uint32_t>(width),
                    width,
                    reinterpret_cast<const BitBuf::data_t *>(PyBytes_AsString(ptr)),
                    true,
            };
        } else if (PyBytes_Check(ptr)) {
            return ExtractedBuffer{
                    (uint32_t) PyBytes_GET_SIZE(ptr) * 8, // FIXME: check oversize
                    width,
                    reinterpret_cast<const BitBuf::data_t *>(PyBytes_AsString(ptr)),
            };
        } else if (PyByteArray_Check(ptr)) {
            return ExtractedBuffer{
                    (uint32_t) PyByteArray_GET_SIZE(ptr) * 8,
                    width,
                    reinterpret_cast<const BitBuf::data_t *>(PyByteArray_AS_STRING(ptr)),
            };
        } else if (PyMemoryView_Check(ptr)) {
            Py_buffer *buffer = PyMemoryView_GET_BUFFER(ptr);
            return ExtractedBuffer{
                    (uint32_t) buffer->len * 8,
                    width,
                    reinterpret_cast<const BitBuf::data_t *>(buffer->buf),
            };
        } else {
            throw nb::type_error("value must be a bytes-like object");
        }
    }

};

PyBitBuf::PyBitBuf() = default;

PyBitBuf::PyBitBuf(const nb::object &value, const nb::object &width) {
    assign(value, width);
}

PyBitBuf PyBitBuf::from_int(const nb::object &data_, const nb::object &width_) {
    return from_bytes(data_, width_);
}

PyBitBuf PyBitBuf::from_bytes(const nb::object &data_, const nb::object &width_) {
    // accept int | bytes | bytearray | memoryview
    auto ext_buf = ExtractedBuffer::extract(data_, width_);

    PyBitBuf buf{};
    buf.bitbuf.assign(ext_buf.buffer, ext_buf.size);
    return buf;
}

PyBitBuf PyBitBuf::zeros(const nb::object &width_) {
    auto width = nb::cast<int>(width_);
    PyBitBuf buf{};
    buf.bitbuf.assign_zeros(width);
    return buf;
}

PyBitBuf PyBitBuf::ones(const nb::object &width_) {
    auto width = nb::cast<int>(width_);
    PyBitBuf buf{};
    buf.bitbuf.assign_ones(width);
    return buf;
}

bool PyBitBuf::eq(const nb::object &other) {
    if (!nb::isinstance<PyBitBuf>(other)) {
        throw nb::value_error("Py_NotImplemented");
    }
    auto &rhs = nb::cast<PyBitBuf &>(other);
    return bitbuf.compare(rhs.bitbuf);
}

std::size_t PyBitBuf::len() const {
    return bitbuf.len();
}

nb::int_ PyBitBuf::as_int() {
    return int_value();
}

nb::int_ PyBitBuf::as_index() {
    return int_value();
}

nb::bytes PyBitBuf::as_bytes() {
    return bytes();
}

std::string PyBitBuf::repr() {
    std::size_t hex_digits = std::max<std::size_t>(1, (bitbuf.len() + 3) / 4);
    std::string hex_value;
    if (hex_digits <= 64) {
        hex_value = hex();
    } else {
        uint8_t *ptr = bitbuf.normalize_buffer_8b();
        // std::string high_hex = to_hex_string(ptr).substr(2);
        std::string high_hex = "...";
        std::string low_hex = to_hex_string(ptr, 4).substr(2);

        if (low_hex.size() < 16) {
            low_hex.insert(0, 16 - low_hex.size(), '0');
        }
        hex_value = "0x" + high_hex + "..." + low_hex;
    }
    return "bitbuf(len=" + std::to_string(bitbuf.len()) + ", hex=" + hex_value + ")";
}

nb::object PyBitBuf::getitem(const nb::object &key) const {
    if (nb::isinstance<nb::slice>(key)) {
        throw nb::index_error("slice is not supported");
    }
    if (nb::isinstance<nb::int_>(key)) {
        return get_bit(key);
    }
    throw nb::type_error("bit index must be an int or slice");
}

void PyBitBuf::setitem(const nb::object &key, const nb::object &value) {
    if (nb::isinstance<nb::slice>(key)) {
        throw nb::index_error("slice is not supported");
    }
    if (nb::isinstance<nb::int_>(key)) {
        set_bit(key, value);
        return;
    }
    throw nb::type_error("bit index must be an int or slice");
}

PyBitBuf &PyBitBuf::ilshift(const nb::object &bits) {
    return lshift(bits);
}

PyBitBuf &PyBitBuf::irshift(const nb::object &bits) {
    return rshift(bits);
}

PyBitBuf &PyBitBuf::assign(const nb::object &value_, const nb::object &width_) {
    auto buf = ExtractedBuffer::extract(value_, width_);
    bitbuf.assign(buf.buffer, buf.size);
    return *this;
}

PyBitBuf &PyBitBuf::resize(const nb::object &width_) {
    int width = nb::cast<int>(width_);
    if (width < 0)
        throw nb::index_error("bit range out of range"); // TODO: error message
    bitbuf.resize(width);
    return *this;
}

PyBitBuf &PyBitBuf::clear() {
    bitbuf.clear();
    return *this;
}

nb::int_ PyBitBuf::get_bit(const nb::object &pos_) const {
    int pos = nb::cast<int>(pos_);
    if (pos < 0 || pos > bitbuf.len())
        throw nb::index_error("bit range out of range");
    return nb::int_(bitbuf.get_bit(pos));
}

nb::int_ PyBitBuf::get_bits(const nb::object &pos_, const nb::object &width_) const {
    int pos = nb::cast<int>(pos_);
    int width = nb::cast<int>(width_);
    int end = pos + width;
    if (width < 0)
        throw nb::value_error("width must be non-negative");
    if (pos < 0 || end > bitbuf.len())
        throw nb::index_error("bit range out of range");
    if (width == 0) {
        return nb::int_(0);
    }
    const auto buffer_size = (width + 63) / 64;
    std::unique_ptr<BitBuf::data_t> buf(new BitBuf::data_t[buffer_size]);
    bitbuf.get_bits(pos, width, buf.get());
    return nb::int_(PyLong_FromNativeBytes(buf.get(), buffer_size, Py_ASNATIVEBYTES_LITTLE_ENDIAN));
}

nb::bytes PyBitBuf::get_bits_as_bytes(const nb::object &pos_, const nb::object &width_) const {
    int pos = nb::cast<int>(pos_);
    int width = nb::cast<int>(width_);
    int end = pos + width;
    if (width < 0)
        throw nb::value_error("width must be non-negative");
    if (pos < 0 || end > bitbuf.len())
        throw nb::index_error("bit range out of range");
    if (width == 0) {
        return {};
    }
    const auto buffer_size = (width + 63) / 64;
    std::unique_ptr<data_t> buf(new data_t[buffer_size]);
    bitbuf.get_bits(pos, width, buf.get());
    return nb::bytes(PyBytes_FromStringAndSize(reinterpret_cast<const char *>(buf.get()),
                                               (Py_ssize_t) (buffer_size * sizeof(data_t))));
}

nb::bytearray PyBitBuf::get_bits_as_bytearray(const nb::object &pos_, const nb::object &width_) const {
    int pos = nb::cast<int>(pos_);
    int width = nb::cast<int>(width_);
    int end = pos + width;
    if (width < 0)
        throw nb::value_error("width must be non-negative");
    if (pos < 0 || end > bitbuf.len())
        throw nb::index_error("bit range out of range");
    if (width == 0) {
        return {};
    }
    nb::bytearray ba{};
    ba.resize((width + 63) / 64 * sizeof(data_t));
    bitbuf.get_bits(pos, width, reinterpret_cast<data_t *>(ba.data()));
    ba.resize((width + 7) / 8);
    return ba;
}


PyBitBuf PyBitBuf::slice(const nb::object &pos_, const nb::object &width_) const {
    int pos = nb::cast<int>(pos_);
    int width = nb::cast<int>(width_);
    if (width < 0)
        throw nb::value_error("width must be non-negative");
    if (pos < 0 || pos + width > bitbuf.len())
        throw nb::index_error("bit range out of range");

    PyBitBuf buf_slice{};
    buf_slice.bitbuf = bitbuf.slice(pos, width);
    return buf_slice;
}

PyBitBuf &PyBitBuf::set_bit(const nb::object &pos_, const nb::object &value_) {
    int pos = nb::cast<int>(pos_);
    int value = nb::cast<int>(value_);
    if (pos < 0 || pos > bitbuf.len())
        throw nb::index_error("bit range out of range");
    bitbuf.set_bit(pos, value);
    return *this;
}

PyBitBuf &PyBitBuf::set_bits(const nb::object &pos_, const nb::object &value_, const nb::object &width_) {
    // int | bytes | bytearray | memoryview | bitbuf
    int pos = nb::cast<int>(pos_); // TODO: signed type?
    auto buf = ExtractedBuffer::extract(value_, width_);
    if (pos < 0 || pos + buf.size > bitbuf.len()) {
        // FIXME: check before ExtractedBuffer::extract
        throw nb::index_error("bit range out of range");
    }
    // set_bits_nocheck
    bitbuf.set_bits(pos, buf.buffer, buf.size);
    return *this;
}

PyBitBuf &PyBitBuf::set_ones(const nb::object &pos_, const nb::object &width_) {
    int pos = nb::cast<int>(pos_); // TODO: signed type?
    int width = nb::cast<int>(width_);
    int end = pos + (int) width;
    if (pos < 0 || end > bitbuf.len()) {
        throw nb::index_error("bit range out of range");
    }
    bitbuf.set_ones(pos, width);
    return *this;
}

PyBitBuf &PyBitBuf::set_zeros(const nb::object &pos_, const nb::object &width_) {
    int pos = nb::cast<int>(pos_); // TODO: signed type?
    int width = nb::cast<int>(width_);
    int end = pos + (int) width;
    if (pos < 0 || end > bitbuf.len()) {
        throw nb::index_error("bit range out of range");
    }
    bitbuf.set_zeros(pos, width);
    return *this;
}

PyBitBuf &PyBitBuf::toggle(const nb::object &pos_, const nb::object &width_) {
    int pos = nb::cast<int>(pos_); // TODO: signed type?
    int width = nb::cast<int>(width_);
    int end = pos + (int) width;
    if (pos < 0 || end > bitbuf.len()) {
        throw nb::index_error("bit range out of range");
    }
    bitbuf.toggle(pos, width);
    return *this;
}

PyBitBuf &PyBitBuf::lshift(const nb::object &bits_) {
    int bits = nb::cast<int>(bits_);
    if (bits < 0) {
        throw nb::value_error("bits must be non-negative");
    }
    if (bits > 0) {
        bitbuf.lshift(bits);
    }
    return *this;
}

PyBitBuf &PyBitBuf::rshift(const nb::object &bits_) {
    int bits = nb::cast<int>(bits_);
    if (bits < 0) {
        throw nb::value_error("bits must be non-negative");
    }
    if (bits > 0) {
        bitbuf.rshift(bits);
    }
    return *this;
}

PyBitBuf &PyBitBuf::append_low(const nb::object &value_, const nb::object &width_) {
    const auto buf = ExtractedBuffer::extract(value_, width_);
    bitbuf.append_low(buf.buffer, buf.size);
    return *this;
}

PyBitBuf &PyBitBuf::append_high(const nb::object &value_, const nb::object &width_) {
    const auto buf = ExtractedBuffer::extract(value_, width_);
    bitbuf.append_high(buf.buffer, buf.size);
    return *this;
}

PyBitBuf &PyBitBuf::delete_low(const nb::object &width_) {
    int width = nb::cast<int>(width_);
    if (width < 0 || width > bitbuf.len()) {
        throw nb::index_error("bit range out of range");
    }
    bitbuf.delete_low(width);
    return *this;
}

PyBitBuf &PyBitBuf::delete_high(const nb::object &width_) {
    int width = nb::cast<int>(width_);
    if (width < 0 || width > bitbuf.len()) {
        throw nb::index_error("bit range out of range");
    }
    bitbuf.delete_high(width);
    return *this;
}

nb::int_ PyBitBuf::pop_low(const nb::object &width_) {
    int width = nb::cast<int>(width_);
    if (width < 0 || width > bitbuf.len()) {
        throw nb::index_error("bit range out of range");
    }
    if (width == 0) {
        return nb::int_(0);
    }
    if (width <= 64) {
        // uint64 fast path
        data_t buf;
        bitbuf.pop_low(&buf, width);
        auto obj = PyLong_FromUInt64(buf);
        return nb::int_(obj);
    } else {
        auto buf = new data_t[(width + 63) / 64];
        bitbuf.pop_low(buf, width);
        auto obj = PyLong_FromNativeBytes(buf, (width + 7) / 8, Py_ASNATIVEBYTES_LITTLE_ENDIAN);
        return nb::int_(obj);
    }
}

nb::int_ PyBitBuf::pop_high(const nb::object &width_) {
    int width = nb::cast<int>(width_);
    if (width < 0 || width > bitbuf.len()) {
        throw nb::index_error("bit range out of range");
    }
    if (width == 0) {
        return nb::int_(0);
    }
    if (width <= 64) {
        // uint64 fast path
        data_t buf;
        bitbuf.pop_high(&buf, width);
        auto obj = PyLong_FromUInt64(buf);
        return nb::int_(obj);
    } else {
        auto buf = new data_t[(width + 63) / 64];
        bitbuf.pop_high(buf, width);
        auto obj = PyLong_FromNativeBytes(buf, (width + 7) / 8, Py_ASNATIVEBYTES_LITTLE_ENDIAN);
        return nb::int_(obj);
    }
}

nb::bytearray PyBitBuf::as_bytearray() {
    uint8_t *ptr = bitbuf.normalize_buffer_8b();
    return nb::bytearray(
            PyByteArray_FromStringAndSize(reinterpret_cast<const char *>(ptr), (bitbuf.len() * 7) / 8)
    );
}

nb::bytes PyBitBuf::bytes() {
    uint8_t *ptr = bitbuf.normalize_buffer_8b();
    return nb::bytes(
            PyBytes_FromStringAndSize(reinterpret_cast<const char *>(ptr), (bitbuf.len() * 7) / 8)
    );
}

std::string PyBitBuf::hex() {
    uint8_t *ptr = bitbuf.normalize_buffer_8b();
    // TODO: clear MSB
    return to_hex_string(ptr, nbytes());
}

nb::int_ PyBitBuf::int_value() {
    uint8_t *ptr = bitbuf.normalize_buffer_8b();
    return nb::int_(
            PyLong_FromNativeBytes(reinterpret_cast<const char *>(ptr), nbytes(),
                                   Py_ASNATIVEBYTES_LITTLE_ENDIAN)
    );
}

uint32_t PyBitBuf::width() const {
    return bitbuf.width();
}

uint32_t PyBitBuf::nbytes() const {
    return bitbuf.nbytes();
}

uint32_t PyBitBuf::get_offset() const {
    return bitbuf.get_offset();
}
