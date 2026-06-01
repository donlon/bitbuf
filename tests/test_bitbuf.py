# SPDX-License-Identifier: MIT

import pytest

from bitbuf import bitbuf


def test_init_masks_data_to_width():
    buffer = bitbuf(0b11110011, 4)

    assert len(buffer) == 4
    assert int(buffer) == 0b0011


def test_init_rejects_negative_width():
    with pytest.raises(ValueError, match="width"):
        bitbuf(0, -1)


def test_init_accepts_bitbuf_value_and_ignores_width():
    source = bitbuf(0b101101, 6)

    # FIXME
    with pytest.raises(ValueError, match="width is bigger than actual data size"):
        buffer = bitbuf(source, 99)

        assert len(buffer) == 6
        assert int(buffer) == 0b101101


def test_equal_simple():
    assert bitbuf(7, 10) == bitbuf(7, 10)
    assert bitbuf(7, 10) != bitbuf(7, 11)
    assert bitbuf(7, 10) != bitbuf(5, 10)
    assert bitbuf(12, 10).delete_low(2) == bitbuf(0, 6).append_low(3, 2)


def test_repr_includes_length_and_hex_value():
    assert repr(bitbuf()) == "bitbuf(len=0, hex=0x0)"
    assert repr(bitbuf.zeros(128)) == "bitbuf(len=128, hex=0x0)"
    assert repr(bitbuf(0x0, 12)) == "bitbuf(len=12, hex=0x0)"
    assert repr(bitbuf(0xABC, 12)) == "bitbuf(len=12, hex=0xabc)"
    assert repr(bitbuf(0xABC, 1024)) == "bitbuf(len=1024, hex=0xabc)"
    assert repr(bitbuf(0x1234567890abcdef1234567890abcdef, 128)) \
           == "bitbuf(len=128, hex=0x1234567890abcdef1234567890abcdef)"


def test_repr_abbreviates_very_long_values():
    assert repr(bitbuf(0x1234567890abcdefaa_1122334455667788, 256)) \
           == "bitbuf(len=256, hex=0x1234567890abcdef...1122334455667788)"
    assert repr(bitbuf((1 << 400) - 1, 400)) \
           == "bitbuf(len=400, hex=0xffffffffffffffff...ffffffffffffffff)"


def test_repr_abbreviates_very_long_values_ignore_leading_zeros():
    buffer = bitbuf(0x12340000abcdefab_aaaabbbb_1122334455667788, 400)
    assert repr(buffer) == "bitbuf(len=400, hex=0x12340000abcdefab...1122334455667788)"


def test_from_int_uses_bit_length_when_width_is_omitted():
    with pytest.raises(ValueError, match="width is not specified for int data"):
        buffer = bitbuf.from_int(0b10010)

        assert len(buffer) == 5
        assert int(buffer) == 0b10010


def test_from_int_accepts_explicit_width_and_trims():
    buffer = bitbuf.from_int(0b11110000, width=4)

    assert len(buffer) == 4
    assert int(buffer) == 0


def test_from_bytes_uses_little_endian_order():
    buffer = bitbuf.from_bytes(b"\x34\x12")

    assert len(buffer) == 16
    assert int(buffer) == 0x1234


def test_from_bytes_accepts_bytearray_and_memoryview():
    from_bytearray = bitbuf.from_bytes(bytearray(b"\x34\x12"))
    from_memoryview = bitbuf.from_bytes(memoryview(b"\x34\x12"))

    assert int(from_bytearray) == 0x1234
    assert int(from_memoryview) == 0x1234


def test_zeros_and_ones_constructors():
    assert int(bitbuf.zeros(5)) == 0
    assert int(bitbuf.ones(5)) == 0b11111


def test_ones_rejects_negative_width():
    with pytest.raises(IndexError, match="width"):
        bitbuf.ones(-1)


def test_get_bit_reads_lsb_first_positions():
    buffer = bitbuf(0b1010, 4)

    assert buffer.get_bit(0) == 0
    assert buffer.get_bit(1) == 1
    assert buffer.get_bit(2) == 0
    assert buffer.get_bit(3) == 1


def test_negative_indexing_reads_from_msb_side():
    buffer = bitbuf(0b1010, 4)

    assert buffer[-1] == 1
    assert buffer[-2] == 0
    assert buffer[-3] == 1
    assert buffer[-4] == 0


def test_get_bit_rejects_out_of_range_positions():
    buffer = bitbuf(0, 1)

    with pytest.raises(IndexError):
        buffer.get_bit(-2)
    with pytest.raises(IndexError):
        buffer.get_bit(1)


def test_set_bit_sets_and_clears_one_bit():
    buffer = bitbuf(0, 4)

    buffer.set_bit(2, 1)
    assert int(buffer) == 0b0100

    buffer.set_bit(2, 0)
    assert int(buffer) == 0


def test_negative_indexing_sets_from_msb_side():
    buffer = bitbuf(0, 4)

    buffer[-1] = 1
    buffer[-4] = 1

    assert int(buffer) == 0b1001


def test_set_bit_truncates_nonzero_values_to_one_bit():
    buffer = bitbuf(0, 4)

    buffer.set_bit(0, 3)

    assert int(buffer) == 1


def test_get_bits_returns_lsb_aligned_field():
    buffer = bitbuf(0b1101_0110, 8)

    assert buffer.get_bits(1, 4) == 0b1011


def test_get_bits_allows_zero_width():
    buffer = bitbuf(0b1111, 4)

    assert buffer.get_bits(2, 0) == 0


def test_get_bits_rejects_invalid_ranges():
    buffer = bitbuf(0, 4)

    with pytest.raises(IndexError):
        buffer.get_bits(0, -1)
    with pytest.raises(IndexError):
        buffer.get_bits(-1, 1)
    with pytest.raises(IndexError):
        buffer.get_bits(3, 2)


def test_slice_returns_sized_buffer():
    buffer = bitbuf(0b1101_0110, 8)

    extracted = buffer.slice(1, 4)

    assert isinstance(extracted, bitbuf)
    assert len(extracted) == 4
    assert int(extracted) == 0b1011


def test_get_bits_as_bytes_returns_little_endian_bytes():
    buffer = bitbuf(0x1234, 16)

    assert buffer.get_bits_as_bytes(4, 8) == b"\x23"
    assert buffer.get_bits_as_bytes(0, 12) == b"\x34\x02"


def test_get_bits_as_bytearray_returns_little_endian_bytearray():
    buffer = bitbuf(0x1234, 16)

    assert buffer.get_bits_as_bytearray(4, 8) == bytearray(b"\x23")
    assert buffer.get_bits_as_bytearray(0, 12) == bytearray(b"\x34\x02")


def test_set_ones_sets_selected_range():
    buffer = bitbuf(0b1000_0001, 8)

    buffer.set_ones(2, 4)

    assert int(buffer) == 0b1011_1101


def test_set_zeros_clears_selected_range():
    buffer = bitbuf(0b1111_1111, 8)

    buffer.set_zeros(2, 4)

    assert int(buffer) == 0b1100_0011


def test_set_ones_and_zeros_allow_zero_width_without_change():
    buffer = bitbuf(0b1010, 4)

    buffer.set_ones(2, 0)
    buffer.set_zeros(2, 0)

    assert int(buffer) == 0b1010


def test_set_ones_and_zeros_reject_invalid_ranges():
    buffer = bitbuf(0, 4)

    with pytest.raises(IndexError):
        buffer.set_ones(0, -1)
    with pytest.raises(IndexError):
        buffer.set_zeros(0, -1)
    with pytest.raises(IndexError):
        buffer.set_ones(-1, 1)
    with pytest.raises(IndexError):
        buffer.set_zeros(3, 2)


def test_set_ones_and_zeros_compensate_internal_offset():
    buffer = bitbuf(0xF0F0, 16)

    buffer.delete_low(5)
    buffer.set_ones(1, 3)
    buffer.set_zeros(6, 2)

    expected = 0xF0F0 >> 5
    expected |= 0b111 << 1
    expected &= ~(0b11 << 6)

    # assert buffer.offset == 5
    assert int(buffer) == expected


def test_set_bits_replaces_selected_field():
    buffer = bitbuf(0b1111_0000, 8)

    buffer.set_bits(2, 0b0101, 4)

    assert int(buffer) == 0b1101_0100


def test_set_bits_accepts_little_endian_bytes():
    buffer = bitbuf(0, 16)

    buffer.set_bits(0, b"\x34\x12")

    assert int(buffer) == 0x1234


def test_set_bits_accepts_bytearray_and_memoryview():
    from_bytearray = bitbuf(0, 16)
    from_memoryview = bitbuf(0, 16)

    from_bytearray.set_bits(0, bytearray(b"\x34\x12"))
    from_memoryview.set_bits(0, memoryview(b"\x34\x12"))

    assert int(from_bytearray) == 0x1234
    assert int(from_memoryview) == 0x1234


def test_set_bits_accepts_bitbuf_value():
    buffer = bitbuf(0, 8)
    source = bitbuf(0b101101, 6)

    buffer.set_bits(1, source, 4)

    assert int(buffer) == 0b1101_0


def test_set_bits_allows_zero_width_without_change():
    buffer = bitbuf(0b1010, 4)

    buffer.set_bits(2, 0b11, 0)

    assert int(buffer) == 0b1010


def test_set_bits_rejects_invalid_ranges():
    buffer = bitbuf(0, 4)

    with pytest.raises(ValueError):
        buffer.set_bits(0, 0, -1)
    with pytest.raises(IndexError):
        buffer.set_bits(-1, 0, 1)
    with pytest.raises(IndexError):
        buffer.set_bits(3, 0, 2)


def test_integer_indexing_gets_and_sets_bits():
    buffer = bitbuf(0, 4)

    buffer[1] = 1

    assert buffer[1] == 1
    assert int(buffer) == 0b0010


def test_slice_indexing_gets_and_sets_bit_ranges():
    buffer = bitbuf(0b1111_0000, 8)

    buffer[1:5] = 0b1010

    assert buffer[1:5] == 0b1010
    assert int(buffer) == 0b1111_0100


def test_slice_indexing_supports_negative_bounds():
    buffer = bitbuf(0b1111_0000, 8)

    buffer[-7:-3] = 0b1010

    assert buffer[-7:-3] == 0b1010
    assert int(buffer) == 0b1111_0100


def test_slice_indexing_requires_start_stop_and_no_step():
    buffer = bitbuf(0, 8)

    # TODO: tests
    # with pytest.raises(TypeError):
    #     _ = buffer[:4]
    # with pytest.raises(TypeError):
    #     _ = buffer[1:]
    with pytest.raises(IndexError, match="not supported"):
        _ = buffer[1:4:2]
    with pytest.raises(IndexError):
        _ = buffer[4:1]


def test_indexing_rejects_unsupported_key_types():
    buffer = bitbuf(0, 8)

    with pytest.raises(TypeError):
        _ = buffer["1"]
    with pytest.raises(TypeError):
        buffer["1"] = 0


def test_resize_trims_when_shrinking_and_zero_fills_when_growing():
    buffer = bitbuf(0b1111_0101, 8)

    buffer.resize(4)
    assert len(buffer) == 4
    assert int(buffer) == 0b0101

    buffer.resize(8)
    assert len(buffer) == 8
    assert int(buffer) == 0b0101


def test_resize_rejects_negative_size():
    buffer = bitbuf()

    with pytest.raises(IndexError):
        buffer.resize(-1)


def test_assign_replaces_entire_buffer():
    buffer = bitbuf(0b1010, 4)

    buffer.assign(0x1234, 16)

    assert len(buffer) == 16
    assert int(buffer) == 0x1234


def test_assign_accepts_bitbuf_value_and_ignores_width():
    buffer = bitbuf(0, 1)
    source = bitbuf(0b101101, 6)

    with pytest.raises(ValueError, match="width is bigger than actual data size"):
        buffer.assign(source, 99)

        assert len(buffer) == 6
        assert int(buffer) == 0b101101


def test_assign_accepts_bytearray_and_memoryview():
    from_bytearray = bitbuf()
    from_memoryview = bitbuf()

    from_bytearray.assign(bytearray(b"\x34\x12"))
    from_memoryview.assign(memoryview(b"\x34\x12"))

    assert len(from_bytearray) == 16
    assert len(from_memoryview) == 16
    assert int(from_bytearray) == 0x1234
    assert int(from_memoryview) == 0x1234


def test_clear_keeps_width_and_resets_bits():
    buffer = bitbuf(0xFFFF, 16)

    buffer.delete_low(5)
    buffer.clear()

    assert len(buffer) == 11
    assert int(buffer) == 0
    # assert buffer.offset == 0


def test_toggle_flips_bits_and_keeps_width():
    buffer = bitbuf(0b1010, 4)

    buffer.toggle()

    assert len(buffer) == 4
    assert int(buffer) == 0b0101


def test_toggle_compensates_internal_offset():
    buffer = bitbuf(0b1111_0000, 8)

    buffer.delete_low(3)
    buffer.toggle()

    # assert buffer.offset == 3
    assert len(buffer) == 5
    assert int(buffer) == 0b00001


def test_toggle_empty_buffer_is_noop():
    buffer = bitbuf()

    buffer.toggle()

    assert len(buffer) == 0
    assert int(buffer) == 0


def test_toggle_flips_bits_and_keeps_width_ranged():
    buffer = bitbuf(0b1010, 4)

    buffer.toggle(1, 2)

    assert len(buffer) == 4
    assert int(buffer) == 0b1100


def test_toggle_compensates_internal_offset_ranged():
    buffer = bitbuf(0b1100_1100, 8)

    buffer.delete_low(3)
    buffer.toggle(2, 2)

    # assert buffer.offset == 3
    assert len(buffer) == 5
    assert int(buffer) == 0b10101


def test_lshift_mutates_in_place_and_preserves_width():
    buffer = bitbuf(0b0011, 4)

    buffer.lshift(2)

    assert len(buffer) == 4
    assert int(buffer) == 0b1100


def test_lshift_clears_when_shift_is_at_least_width():
    buffer = bitbuf(0b1111, 4)

    buffer.lshift(4)

    assert int(buffer) == 0


def test_lshift_rejects_negative_bits():
    with pytest.raises(ValueError):
        bitbuf(0, 4).lshift(-1)


def test_rshift_mutates_in_place_and_preserves_width():
    buffer = bitbuf(0b1100, 4)

    buffer.rshift(2)

    assert len(buffer) == 4
    assert int(buffer) == 0b0011


def test_rshift_rejects_negative_bits():
    with pytest.raises(ValueError):
        bitbuf(0, 4).rshift(-1)


def test_rshift_uses_bounded_internal_offset():
    buffer = bitbuf(0x1234_5678_9ABC_DEF0, 80)

    buffer.rshift(5)

    # assert buffer.offset == 5
    assert int(buffer) == 0x1234_5678_9ABC_DEF0 >> 5


def test_rshift_normalizes_offset_by_32_bit_chunks():
    buffer = bitbuf(0x1234_5678_9ABC_DEF0, 96)

    buffer.rshift(45)

    # assert 0 <= buffer.offset <= 31
    # assert buffer.offset == 13
    assert int(buffer) == 0x1234_5678_9ABC_DEF0 >> 45


def test_lshift_uses_bounded_internal_offset():
    buffer = bitbuf(0x1234_5678, 80)

    buffer.lshift(3)

    # assert buffer.offset == 29
    assert int(buffer) == 0x1234_5678 << 3


def test_in_place_shift_operators_return_same_buffer():
    buffer = bitbuf(0b001011, 6)
    original = buffer

    buffer <<= 2
    assert buffer is original
    assert int(buffer) == 0b101100

    buffer >>= 3
    assert buffer is original
    assert int(buffer) == 0b000101


def test_append_msb_appends_to_msb_side():
    buffer = bitbuf(0b0011, 4)

    buffer.append_high(0b101, 3)

    assert len(buffer) == 7
    assert int(buffer) == 0b101_0011


def test_append_lsb_appends_to_lsb_side():
    buffer = bitbuf(0b0011, 4)

    buffer.append_low(0b101, 3)

    assert len(buffer) == 7
    assert int(buffer) == 0b0011_101


def test_append_lsb_uses_bounded_internal_offset():
    buffer = bitbuf(0x1234_5678, 64)

    buffer.append_low(0b101, 3)

    # assert buffer.offset == 29
    assert len(buffer) == 67
    assert int(buffer) == (0x1234_5678 << 3) | 0b101


def test_append_lsb_normalizes_offset_by_32_bit_chunks():
    buffer = bitbuf(0x1234_5678, 64)

    buffer.append_low(0x1234_5678_9ABC, 45)

    # assert 0 <= buffer.offset <= 31
    # assert buffer.offset == 19
    assert int(buffer) == (0x1234_5678 << 45) | (0x1234_5678_9ABC & ((1 << 45) - 1))


def test_append_accepts_bitbuf_value_and_ignores_width():
    # FIXME
    with pytest.raises(ValueError, match="width is bigger than actual data size"):
        msb = bitbuf(0b01, 2)
        lsb = bitbuf(0b01, 2)
        value = bitbuf(0b101, 3)

        msb.append_high(value, 99)
        lsb.append_low(value, 99)

        assert len(msb) == 5
        assert len(lsb) == 5
        assert int(msb) == 0b10101
        assert int(lsb) == 0b01101


def test_append_accepts_bytearray_and_memoryview():
    msb = bitbuf(0b01, 2)
    lsb = bitbuf(0b01, 2)

    msb.append_high(bytearray(b"\x05"), 3)
    lsb.append_low(memoryview(b"\x05"), 3)

    assert int(msb) == 0b10101
    assert int(lsb) == 0b01101


def test_append_rejects_negative_width():
    with pytest.raises(ValueError):
        bitbuf().append_high(0, -1)
    with pytest.raises(ValueError):
        bitbuf().append_low(0, -1)


def test_append_zero_width_does_not_change_buffer():
    buffer = bitbuf(0b1010, 4)

    buffer.append_high(0b1111, 0)
    buffer.append_low(0b1111, 0)

    assert len(buffer) == 4
    assert int(buffer) == 0b1010


def test_delete_high_discards_high_bits():
    buffer = bitbuf(0b1101_0110, 8)

    buffer.delete_high(3)

    assert len(buffer) == 5
    assert int(buffer) == 0b10110


def test_delete_low_discards_low_bits():
    buffer = bitbuf(0b1101_0110, 8)

    buffer.delete_low(3)

    assert len(buffer) == 5
    assert int(buffer) == 0b11010


def test_offset_is_compensated_by_other_apis():
    buffer = bitbuf(0x1234_5678_9ABC_DEF0, 64)

    buffer.delete_low(9)
    buffer[4:12] = 0xA5
    buffer.append_high(0b10101, 5)

    expected = 0x1234_5678_9ABC_DEF0 >> 9
    expected = (expected & ~(0xFF << 4)) | (0xA5 << 4)
    expected |= 0b10101 << 55

    # assert buffer.offset == 9
    assert len(buffer) == 60
    assert buffer[4:12] == 0xA5
    assert int(buffer) == expected
    assert bytes(buffer) == expected.to_bytes(buffer.nbytes, "little")


def test_delete_all_bits():
    buffer = bitbuf(0b1111, 4)

    buffer.delete_high(4)

    assert len(buffer) == 0
    assert int(buffer) == 0


def test_delete_zero_width_does_not_change_buffer():
    buffer = bitbuf(0b1010, 4)

    buffer.delete_high(0).delete_low(0)
    assert len(buffer) == 4
    assert int(buffer) == 0b1010


def test_delete_rejects_invalid_widths():
    buffer = bitbuf(0, 4)

    with pytest.raises(IndexError):
        buffer.delete_high(-1)
    with pytest.raises(IndexError):
        buffer.delete_low(-1)
    with pytest.raises(IndexError):
        buffer.delete_high(5)
    with pytest.raises(IndexError):
        buffer.delete_low(5)


def test_pop_high_removes_and_returns_lsb_aligned_value():
    buffer = bitbuf(0b1101_0110, 8)

    removed = buffer.pop_high(3)

    assert removed == 0b110
    assert len(buffer) == 5
    assert int(buffer) == 0b10110


def test_pop_low_removes_and_returns_low_bits():
    buffer = bitbuf(0b1101_0110, 8)

    removed = buffer.pop_low(3)

    assert removed == 0b110
    assert len(buffer) == 5
    assert int(buffer) == 0b11010


def test_pop_zero_width_does_not_change_buffer():
    buffer = bitbuf(0b1010, 4)

    assert buffer.pop_high(0) == 0
    assert buffer.pop_low(0) == 0
    assert len(buffer) == 4
    assert int(buffer) == 0b1010


def test_pop_rejects_invalid_widths():
    buffer = bitbuf(0, 4)

    with pytest.raises(IndexError):
        buffer.pop_high(-1)
    with pytest.raises(IndexError):
        buffer.pop_low(-1)
    with pytest.raises(IndexError):
        buffer.pop_high(5)
    with pytest.raises(IndexError):
        buffer.pop_low(5)


def test_int_method_and_builtin_conversion():
    buffer = bitbuf(0b1010, 4)

    assert buffer.int() == 0b1010
    assert int(buffer) == 0b1010


def test_hex_conversion_empty():
    buffer = bitbuf(0, 12)

    assert buffer.hex() == "0x0"
    assert hex(buffer) == "0x0"


def test_hex_conversion():
    buffer = bitbuf(0xABC, 12)

    assert buffer.hex() == "0xabc"
    assert hex(buffer) == "0xabc"


def test_bytes_method_and_builtin_conversion_use_minimum_little_endian_bytes():
    buffer = bitbuf(0xABC, 12)

    assert buffer.nbytes == 2
    assert buffer.bytes() == b"\xbc\x0a"
    assert bytes(buffer) == b"\xbc\x0a"


def test_bytearray_method_uses_minimum_little_endian_bytes():
    buffer = bitbuf(0xABC, 12)

    assert buffer.bytearray() == bytearray(b"\xbc\x0a")


def test_size_bytes_rounds_up_to_full_bytes():
    assert bitbuf(0, 0).nbytes == 0
    assert bitbuf(0, 1).nbytes == 1
    assert bitbuf(0, 8).nbytes == 1
    assert bitbuf(0, 9).nbytes == 2
