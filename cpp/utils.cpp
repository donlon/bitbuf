// SPDX-License-Identifier: MIT

#include "utils.h"

std::string to_hex_string(const uint8_t *ptr, size_t length) {
    static const char *hex_table = "0123456789abcdef";
    if (length == 0) return "0x0";
    std::string str{"0x"};
    str.reserve(2 * length + 4);
    auto i = length - 1;
    bool find_nonzero = false;
    do {
        uint8_t byte = ptr[i];
        if (find_nonzero || byte >> 4) {
            str += hex_table[byte >> 4];
            find_nonzero = true;
        }
        if (find_nonzero || byte & 15) {
            str += hex_table[byte & 15];
            find_nonzero = true;
        }
    } while (i--);
    if (str.length() == 2) str += '0';
    return str;
}

PyObject *create_pylong(const void *buffer, uint32_t length) {
#if PY_MINOR_VERSION >= 14
    return PyLong_FromNativeBytes(reinterpret_cast<const char *>(buffer),
                                  (length + 7) / 8,
                                  Py_ASNATIVEBYTES_LITTLE_ENDIAN | Py_ASNATIVEBYTES_UNSIGNED_BUFFER);
#else
    // TODO: use _PyLong_FromByteArray for version < 3.13 ?
    if (length <= 64) {
        // length == 64 would make `1ull << length` undefined behaviour.
        uint64_t mask = length >= 64 ? ~0ull : ((1ull << length) - 1);
        uint64_t value = reinterpret_cast<const uint64_t *>(buffer)[0] & mask;
        return PyLong_FromUnsignedLongLong(value);
    } else {
        PyObject *bytes = PyBytes_FromStringAndSize((const char *) buffer, (length + 7) / 8);
        auto obj = PyObject_CallMethod((PyObject *) &PyLong_Type, "from_bytes", "Os", bytes, "little");
        Py_DecRef(bytes);
        return obj;
    }
#endif
}
