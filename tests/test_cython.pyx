# SPDX-License-Identifier: MIT

import pytest
from bitbuf import ABI_VERSION, check_abi_version

from bitbuf cimport (
	ABI_VERSION as ABI_VERSION_C,
	bitbuf,
	create_ones,
	create_random,
	create_zeros,
	from_buffer,
)


def test_abi_version():
    expected_version = 1

    check_abi_version(abi_version=expected_version, library_name="test_cython")
    with pytest.raises(ImportError, match=f"Library test_cython requires bitbuf with abi version {expected_version + 10} but"):
        check_abi_version(abi_version=expected_version + 10, library_name="test_cython")

    assert ABI_VERSION == expected_version
    assert ABI_VERSION_C == expected_version


def test_cimported_bitbuf_type_supports_simple_methods():
    cdef bitbuf buffer = bitbuf(0b1011, 4)

    assert len(buffer) == 4
    assert buffer.get_bit(0) == 1
    assert buffer.get_bits(1, 2) == 0b01
    cdef bitbuf toggled = buffer.clone()
    toggled.toggle(0, 2)
    assert int(toggled) == 0b1000


def test_create_zeros():
    cdef bitbuf zeros = create_zeros(5)
    assert len(zeros) == 5
    assert int(zeros) == 0


def test_create_ones():
    cdef bitbuf ones = create_ones(5)
    assert len(ones) == 5
    assert int(ones) == 0b11111

def test_create_random():
    cdef bitbuf buf = create_random(5)
    assert len(buf) == 5


def test_cython_from_buffer_extracts_unaligned_range():
    cdef unsigned char[2] raw = [0x34, 0x12]
    cdef bitbuf buffer = from_buffer(&raw[0], 4, 8)

    assert len(buffer) == 8
    assert int(buffer) == 0x23
