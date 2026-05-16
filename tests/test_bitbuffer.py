import pytest

from bitbuffer import bitbuf


def test_init_masks_data_to_size():
    buffer = bitbuf(4, 0b11110011)

    assert len(buffer) == 4
    assert int(buffer) == 0b0011


def test_init_rejects_negative_size():
    with pytest.raises(ValueError, match="size"):
        bitbuf(-1)


def test_from_int_uses_bit_length_when_size_is_omitted():
    buffer = bitbuf.from_int(0b10010)

    assert len(buffer) == 5
    assert int(buffer) == 0b10010


def test_from_int_accepts_explicit_size_and_trims():
    buffer = bitbuf.from_int(0b11110000, size=4)

    assert len(buffer) == 4
    assert int(buffer) == 0


def test_from_bytes_uses_little_endian_order():
    buffer = bitbuf.from_bytes(b"\x34\x12")

    assert len(buffer) == 16
    assert int(buffer) == 0x1234


def test_zeros_and_ones_constructors():
    assert int(bitbuf.zeros(5)) == 0
    assert int(bitbuf.ones(5)) == 0b11111


def test_ones_rejects_negative_size():
    with pytest.raises(ValueError, match="size"):
        bitbuf.ones(-1)


def test_get_bit_reads_lsb_first_positions():
    buffer = bitbuf(4, 0b1010)

    assert buffer.get_bit(0) == 0
    assert buffer.get_bit(1) == 1
    assert buffer.get_bit(2) == 0
    assert buffer.get_bit(3) == 1


def test_get_bit_rejects_out_of_range_positions():
    buffer = bitbuf(1)

    with pytest.raises(IndexError):
        buffer.get_bit(-1)
    with pytest.raises(IndexError):
        buffer.get_bit(1)


def test_set_bit_sets_and_clears_one_bit():
    buffer = bitbuf(4)

    buffer.set_bit(2, 1)
    assert int(buffer) == 0b0100

    buffer.set_bit(2, 0)
    assert int(buffer) == 0


def test_set_bit_truncates_nonzero_values_to_one_bit():
    buffer = bitbuf(4)

    buffer.set_bit(0, 3)

    assert int(buffer) == 1


def test_get_bits_returns_lsb_aligned_field():
    buffer = bitbuf(8, 0b1101_0110)

    assert buffer.get_bits(1, 4) == 0b1011


def test_get_bits_allows_zero_width():
    buffer = bitbuf(4, 0b1111)

    assert buffer.get_bits(2, 0) == 0


def test_get_bits_rejects_invalid_ranges():
    buffer = bitbuf(4)

    with pytest.raises(ValueError):
        buffer.get_bits(0, -1)
    with pytest.raises(IndexError):
        buffer.get_bits(-1, 1)
    with pytest.raises(IndexError):
        buffer.get_bits(3, 2)


def test_set_bits_replaces_selected_field():
    buffer = bitbuf(8, 0b1111_0000)

    buffer.set_bits(2, 4, 0b0101)

    assert int(buffer) == 0b1101_0100


def test_set_bits_accepts_little_endian_bytes():
    buffer = bitbuf(16)

    buffer.set_bits(0, 16, b"\x34\x12")

    assert int(buffer) == 0x1234


def test_set_bits_allows_zero_width_without_change():
    buffer = bitbuf(4, 0b1010)

    buffer.set_bits(2, 0, 0b11)

    assert int(buffer) == 0b1010


def test_set_bits_rejects_invalid_ranges():
    buffer = bitbuf(4)

    with pytest.raises(ValueError):
        buffer.set_bits(0, -1, 0)
    with pytest.raises(IndexError):
        buffer.set_bits(-1, 1, 0)
    with pytest.raises(IndexError):
        buffer.set_bits(3, 2, 0)


def test_integer_indexing_gets_and_sets_bits():
    buffer = bitbuf(4)

    buffer[1] = 1

    assert buffer[1] == 1
    assert int(buffer) == 0b0010


def test_slice_indexing_gets_and_sets_bit_ranges():
    buffer = bitbuf(8, 0b1111_0000)

    buffer[1:5] = 0b1010

    assert buffer[1:5] == 0b1010
    assert int(buffer) == 0b1111_0100


def test_slice_indexing_requires_start_stop_and_no_step():
    buffer = bitbuf(8)

    with pytest.raises(TypeError):
        _ = buffer[:4]
    with pytest.raises(TypeError):
        _ = buffer[1:]
    with pytest.raises(TypeError):
        _ = buffer[1:4:2]
    with pytest.raises(ValueError):
        _ = buffer[4:1]


def test_indexing_rejects_unsupported_key_types():
    buffer = bitbuf(8)

    with pytest.raises(TypeError):
        _ = buffer["1"]
    with pytest.raises(TypeError):
        buffer["1"] = 0


def test_resize_trims_when_shrinking_and_zero_fills_when_growing():
    buffer = bitbuf(8, 0b1111_0101)

    buffer.resize(4)
    assert len(buffer) == 4
    assert int(buffer) == 0b0101

    buffer.resize(8)
    assert len(buffer) == 8
    assert int(buffer) == 0b0101


def test_resize_rejects_negative_size():
    buffer = bitbuf()

    with pytest.raises(ValueError):
        buffer.resize(-1)


def test_lshift_mutates_in_place_and_preserves_size():
    buffer = bitbuf(4, 0b0011)

    buffer.lshift(2)

    assert len(buffer) == 4
    assert int(buffer) == 0b1100


def test_lshift_clears_when_shift_is_at_least_size():
    buffer = bitbuf(4, 0b1111)

    buffer.lshift(4)

    assert int(buffer) == 0


def test_lshift_rejects_negative_bits():
    with pytest.raises(ValueError):
        bitbuf(4).lshift(-1)


def test_rshift_mutates_in_place_and_preserves_size():
    buffer = bitbuf(4, 0b1100)

    buffer.rshift(2)

    assert len(buffer) == 4
    assert int(buffer) == 0b0011


def test_rshift_rejects_negative_bits():
    with pytest.raises(ValueError):
        bitbuf(4).rshift(-1)


def test_in_place_shift_operators_return_same_buffer():
    buffer = bitbuf(6, 0b001011)
    original = buffer

    buffer <<= 2
    assert buffer is original
    assert int(buffer) == 0b101100

    buffer >>= 3
    assert buffer is original
    assert int(buffer) == 0b000101


def test_lsplice_appends_to_msb_side():
    buffer = bitbuf(4, 0b0011)

    buffer.lsplice(3, 0b101)

    assert len(buffer) == 7
    assert int(buffer) == 0b101_0011


def test_rsplice_appends_to_lsb_side():
    buffer = bitbuf(4, 0b0011)

    buffer.rsplice(3, 0b101)

    assert len(buffer) == 7
    assert int(buffer) == 0b0011_101


def test_append_aliases_match_splice_operations():
    msb = bitbuf(2, 0b01)
    lsb = bitbuf(2, 0b01)

    msb.append_msb(2, 0b10)
    lsb.append_lsb(2, 0b10)

    assert int(msb) == 0b1001
    assert int(lsb) == 0b0110


def test_splice_rejects_negative_width():
    with pytest.raises(ValueError):
        bitbuf().lsplice(-1)
    with pytest.raises(ValueError):
        bitbuf().rsplice(-1)


def test_splice_zero_width_does_not_change_buffer():
    buffer = bitbuf(4, 0b1010)

    buffer.lsplice(0, 0b1111)
    buffer.rsplice(0, 0b1111)

    assert len(buffer) == 4
    assert int(buffer) == 0b1010


def test_delete_msb_removes_high_bits_and_returns_lsb_aligned_value():
    buffer = bitbuf(8, 0b1101_0110)

    removed = buffer.delete_msb(3)

    assert removed == 0b110
    assert len(buffer) == 5
    assert int(buffer) == 0b10110


def test_delete_lsb_removes_low_bits_and_returns_them():
    buffer = bitbuf(8, 0b1101_0110)

    removed = buffer.delete_lsb(3)

    assert removed == 0b110
    assert len(buffer) == 5
    assert int(buffer) == 0b11010


def test_delete_all_bits():
    buffer = bitbuf(4, 0b1111)

    removed = buffer.delete_msb(4)

    assert removed == 0b1111
    assert len(buffer) == 0
    assert int(buffer) == 0


def test_delete_zero_width_does_not_change_buffer():
    buffer = bitbuf(4, 0b1010)

    assert buffer.delete_msb(0) == 0
    assert buffer.delete_lsb(0) == 0
    assert len(buffer) == 4
    assert int(buffer) == 0b1010


def test_delete_rejects_invalid_widths():
    buffer = bitbuf(4)

    with pytest.raises(ValueError):
        buffer.delete_msb(-1)
    with pytest.raises(ValueError):
        buffer.delete_lsb(-1)
    with pytest.raises(IndexError):
        buffer.delete_msb(5)
    with pytest.raises(IndexError):
        buffer.delete_lsb(5)


def test_toint_and_int_conversion():
    buffer = bitbuf(4, 0b1010)

    assert buffer.toint() == 0b1010
    assert int(buffer) == 0b1010


def test_tobytes_and_bytes_conversion_use_minimum_little_endian_bytes():
    buffer = bitbuf(12, 0xABC)

    assert buffer.size_bytes() == 2
    assert buffer.tobytes() == b"\xbc\x0a"
    assert bytes(buffer) == b"\xbc\x0a"


def test_size_bytes_rounds_up_to_full_bytes():
    assert bitbuf(0).size_bytes() == 0
    assert bitbuf(1).size_bytes() == 1
    assert bitbuf(8).size_bytes() == 1
    assert bitbuf(9).size_bytes() == 2
