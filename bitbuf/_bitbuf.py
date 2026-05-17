from __future__ import annotations

from typing import TypeAlias

from ._utils import ensure_int

input_types: TypeAlias = int | bytes | bytearray | memoryview | "bitbuf"


class bitbuf:
    """A mutable little-endian bit buffer backed by Python's ``int``.

    Bits are indexed from least significant to most significant, so pos
    ``0`` refers to the least significant bit of ``data``. The buffer stores an
    explicit bit ``width`` and keeps its integer payload masked to that width.

    Byte conversion uses little-endian order for both input and output.
    """

    def __init__(self, value: input_types = 0, width: int | None = None):
        """
        Args:
            value: Initial buffer contents as an integer, little-endian bytes,
                or another bitbuf.
            width: Initial buffer width in bits. Ignored when ``value`` is a bitbuf.
        """
        data, self._width = self._sized_value(value, width)
        self._offset = 0
        self._data = data
        self._trim()

    @classmethod
    def from_int(cls, data: int, width: int | None = None) -> bitbuf:
        """Create a buffer from an integer."""
        data = ensure_int(data)
        if width is None:
            width = data.bit_length()
        return cls(data, width)

    @classmethod
    def from_bytes(cls, data: bytes | bytearray | memoryview, width: int | None = None) -> bitbuf:
        """Create a buffer from little-endian bytes."""
        if width is None:
            width = len(data) * 8
        return cls(data, width)

    @classmethod
    def zeros(cls, width: int) -> bitbuf:
        """Create a zero-filled buffer."""
        return cls(0, width)

    @classmethod
    def ones(cls, width: int) -> bitbuf:
        """Create a one-filled buffer."""
        if width < 0:
            raise ValueError("width must be non-negative")
        return cls((1 << width) - 1 if width else 0, width)

    def __len__(self) -> int:
        return self._width

    def __int__(self) -> int:
        return self.int()

    def __index__(self) -> int:
        return self.int()

    def __bytes__(self) -> bytes:
        return self.bytes()

    def __repr__(self) -> str:
        return f"bitbuf(len={self._width}, hex={self._repr_hex()})"

    def __getitem__(self, key: int | slice) -> int:
        if isinstance(key, slice):
            start, stop = self._slice_bounds(key)
            return self.get_bits(start, stop - start)
        if isinstance(key, int):
            return self.get_bit(key)
        raise TypeError("bit index must be an int or slice")

    def __setitem__(self, key: int | slice, value: input_types) -> None:
        if isinstance(key, slice):
            start, stop = self._slice_bounds(key)
            self.set_bits(start, value, stop - start)
            return
        if isinstance(key, int):
            self.set_bit(key, value)
            return
        raise TypeError("bit index must be an int or slice")

    def __ilshift__(self, bits: int) -> bitbuf:
        self.lshift(bits)
        return self

    def __irshift__(self, bits: int) -> bitbuf:
        self.rshift(bits)
        return self

    def _mask(self) -> int:
        if self._width <= 0:
            return 0
        return (1 << self._width) - 1

    @staticmethod
    def _sized_value(value: input_types, width: int | None) -> tuple[int, int]:
        if isinstance(value, bitbuf):
            return value.int(), len(value)
        if isinstance(value, (bytes, bytearray, memoryview)):
            data = ensure_int(value)
            if width is None:
                width = len(value) * 8
        else:
            data = ensure_int(value)
            if width is None:
                width = data.bit_length()
        width = int(width)
        if width < 0:
            raise ValueError("width must be non-negative")
        return data, width

    def _trim(self) -> None:
        if self._width == 0:
            self._data = 0
            self._offset = 0
            return
        self._data &= self._mask() << self._offset

    def _increase_offset(self, bits: int) -> None:
        self._offset += bits
        if self._offset >= 32:
            words = self._offset // 32
            self._data >>= words * 32
            self._offset %= 32

    def _decrease_offset(self, bits: int) -> None:
        if bits <= self._offset:
            self._offset -= bits
            return
        words = (bits - self._offset + 31) // 32
        self._data <<= words * 32
        self._offset += words * 32 - bits

    def _slice_bounds(self, key: slice) -> tuple[int, int]:
        if key.step is not None:
            raise TypeError("slice step is not supported")
        if key.start is None or key.stop is None:
            raise TypeError("slice start and stop are required")
        start = self._normalize_position(int(key.start), allow_end=True)
        stop = self._normalize_position(int(key.stop), allow_end=True)
        if stop < start:
            raise ValueError("slice stop must be greater than or equal to start")
        return start, stop

    def _normalize_position(self, pos: int, allow_end: bool = False) -> int:
        if pos < 0:
            pos += self._width
        upper = self._width if allow_end else self._width - 1
        if pos < 0 or pos > upper:
            raise IndexError("bit pos out of range")
        return pos

    def _repr_hex(self) -> str:
        hex_digits = max(1, (self._width + 3) // 4)
        if hex_digits <= 64:
            return self.hex()
        low = self.get_bits(0, 64)
        high_width = min(64, self._width)
        high = self.get_bits(self._width - high_width, high_width)
        return f"0x{high:x}...{low:016x}"

    def assign(self, value: input_types = 0, width: int | None = None) -> None:
        """
        Replace the whole buffer with ``value`` and ``width``.

        Args:
            value: Replacement contents as an integer, little-endian bytes, or
                another bitbuf.
            width: Replacement width in bits. Ignored when ``value`` is a bitbuf.
        """
        data, self._width = self._sized_value(value, width)
        self._offset = 0
        self._data = data
        self._trim()

    def resize(self, width: int) -> None:
        """
        Resize the buffer to ``width`` bits.

        When the new width is smaller than the current width, bits beyond the new
        most significant bit are discarded.

        Args:
            width: Target buffer width in bits.
        """
        self._width = int(width)
        if self._width < 0:
            raise ValueError("width must be non-negative")
        self._trim()

    def clear(self) -> None:
        """Clear all bits while keeping the current width unchanged."""
        self._data = 0
        self._offset = 0

    def get_bit(self, pos: int) -> int:
        """
        Return the bit value at ``pos``.

        Args:
            pos: Bit index, where 0 refers to the least significant bit.
        """
        pos = self._normalize_position(pos)
        return (self._data >> (self._offset + pos)) & 1

    def get_bits(self, pos: int, width: int) -> int:
        """
        Return ``width`` bits starting at ``pos`` as an integer.

        Args:
            pos: Starting bit index for the read.
            width: Number of bits to read.
        """
        if width < 0:
            raise ValueError("width must be non-negative")
        if pos < 0 or pos + width > self._width:
            raise IndexError("bit range out of range")
        if width == 0:
            return 0
        return (self._data >> (self._offset + pos)) & ((1 << width) - 1)

    def get_bits_as_bytes(self, pos: int, width: int) -> bytes:
        """
        Return ``width`` bits starting at ``pos`` as little-endian bytes.
        """
        return self.get_bits(pos, width).to_bytes((width + 7) // 8, "little")

    def get_bits_as_bytearray(self, pos: int, width: int) -> bytearray:
        """
        Return ``width`` bits starting at ``pos`` as little-endian bytearray.
        """
        return bytearray(self.get_bits_as_bytes(pos, width))

    def slice(self, pos: int, width: int) -> bitbuf:
        """
        Return ``width`` bits starting at ``pos`` as a new bitbuf.
        """
        return bitbuf(self.get_bits(pos, width), width)

    def set_bit(self, pos: int, value: int = 1) -> None:
        """
        Set the bit at ``pos`` to ``value``.

        Args:
            pos: Bit index, where 0 refers to the least significant bit.
            value: Bit value to write. Non-zero values are truncated to 1.
        """
        self.set_bits(self._normalize_position(pos), value, 1)

    def set_bits(self, pos: int, value: input_types = 0, width: int | None = None) -> None:
        """
        Replace ``width`` bits starting at ``pos`` with ``value``.

        The least significant bit of ``value`` is written to ``pos``.

        Args:
            pos: Starting bit index for the write.
            value: Replacement value, interpreted in little-endian bit order.
            width: Number of bits to replace. Ignored when ``value`` is a bitbuf.
        """
        value, width = self._sized_value(value, width)
        if pos < 0 or pos + width > self._width:
            raise IndexError("bit range out of range")
        if width == 0:
            return
        pos += self._offset
        mask = ((1 << width) - 1) << pos
        value_mask = (value & ((1 << width) - 1)) << pos
        self._data = (self._data & ~mask) | value_mask

    def set_ones(self, pos: int, width: int) -> None:
        """
        Set ``width`` bits starting at ``pos`` to 1.

        Args:
            pos: Starting bit index for the write.
            width: Number of bits to set.
        """
        if width < 0:
            raise ValueError("width must be non-negative")
        if pos < 0 or pos + width > self._width:
            raise IndexError("bit range out of range")
        if width == 0:
            return
        self._data |= ((1 << width) - 1) << (self._offset + pos)

    def set_zeros(self, pos: int, width: int) -> None:
        """
        Set ``width`` bits starting at ``pos`` to 0.

        Args:
            pos: Starting bit index for the write.
            width: Number of bits to clear.
        """
        if width < 0:
            raise ValueError("width must be non-negative")
        if pos < 0 or pos + width > self._width:
            raise IndexError("bit range out of range")
        if width == 0:
            return
        self._data &= ~(((1 << width) - 1) << (self._offset + pos))

    def toggle(self, pos: int = 0, width: int | None = None) -> None:
        """
        Flip bits of specified range. If width is unassigneed, it toggles all bits starts from ``pos`` up to highest bit.
        """
        if self._width == 0 and pos == 0 and width is None:
            return
        if pos < 0 or pos >= self._width:
            raise IndexError("bit range out of range")
        if width is None:
            # Set width to fill all MSB
            width = self._width - pos
        elif width < 0:
            raise ValueError("width must be non-negative")
        if pos + width > self._width:
            raise IndexError("bit range out of range")
        if width == 0 or self._width == 0:
            return 0
        mask = ((1 << width) - 1) << (pos + self._offset)
        self._data ^= mask

    def lshift(self, bits: int) -> None:
        """
        Shift the buffer left by ``bits`` while keeping the width unchanged.

        Args:
            bits: Number of bit positions to shift toward the most significant end.
        """
        if bits < 0:
            raise ValueError("bits must be non-negative")
        if bits >= self._width:
            self._data = 0
            self._offset = 0
        else:
            self._decrease_offset(bits)
            self._trim()

    def rshift(self, bits: int) -> None:
        """
        Shift the buffer right by ``bits`` while keeping the width unchanged.

        Args:
            bits: Number of bit positions to shift toward the least significant end.
        """
        if bits < 0:
            raise ValueError("bits must be non-negative")
        if bits >= self._width:
            self._data = 0
            self._offset = 0
        else:
            self._increase_offset(bits)
            self._trim()

    def append_low(self, value: input_types = 0, width: int | None = None) -> None:
        """
        Append ``width`` bits to the least significant side and grow the buffer.

        Args:
            value: Bits to append, interpreted in little-endian bit order.
            width: Number of bits to append. Ignored when ``value`` is a bitbuf.
        """
        value, width = self._sized_value(value, width)
        if width == 0:
            return
        value &= (1 << width) - 1
        self._decrease_offset(width)
        self._width += width
        self.set_bits(0, value, width)
        self._trim()

    def append_high(self, value: input_types = 0, width: int | None = None) -> None:
        """
        Append ``width`` bits to the most significant side and grow the buffer.

        Args:
            value: Bits to append, interpreted in little-endian bit order.
            width: Number of bits to append. Ignored when ``value`` is a bitbuf.
        """
        value, width = self._sized_value(value, width)
        value &= (1 << width) - 1 if width > 0 else 0
        self._data |= value << (self._offset + self._width)
        self._width += width

    def delete_low(self, width: int) -> None:
        """Discard ``width`` bits from the least significant side."""
        if width < 0:
            raise ValueError("width must be non-negative")
        if width > self._width:
            raise IndexError("bit range out of range")
        if width == 0:
            return
        self._width -= width
        self._increase_offset(width)
        self._trim()

    def delete_high(self, width: int) -> None:
        """Discard ``width`` bits from the most significant side."""
        if width < 0:
            raise ValueError("width must be non-negative")
        if width > self._width:
            raise IndexError("bit range out of range")
        if width == 0:
            return
        self._width -= width
        self._trim()

    def pop_low(self, width: int) -> int:
        """Remove and return ``width`` bits from the least significant side."""
        if width < 0:
            raise ValueError("width must be non-negative")
        if width > self._width:
            raise IndexError("bit range out of range")
        if width == 0:
            return 0
        value = self.get_bits(0, width)
        self.delete_low(width)
        return value

    def pop_high(self, width: int) -> int:
        """Remove and return ``width`` bits from the most significant side."""
        if width < 0:
            raise ValueError("width must be non-negative")
        if width > self._width:
            raise IndexError("bit range out of range")
        if width == 0:
            return 0
        pos = self._width - width
        value = self.get_bits(pos, width)
        self.delete_high(width)
        return value

    def bytearray(self) -> bytearray:
        """
        Return the buffer contents as little-endian bytearray.
        """
        return bytearray(self.bytes())

    def bytes(self) -> bytes:
        """
        Return the buffer contents as little-endian bytes.

        Returns:
            The current buffer value serialized to the minimum number of bytes.
        """
        return self.int().to_bytes(self.size_bytes, "little")

    def hex(self) -> str:
        """Return ``hex(self.int())``."""
        return hex(self.int())

    def int(self) -> int:
        """
        Return the buffer contents as an integer.

        Returns:
            The current buffer value as a Python integer.
        """
        return (self._data >> self._offset) & self._mask()

    @property
    def width(self) -> int:
        """
        Return the width of the buffer.

        Returns:
            Width of the buffer in bits.
        """
        return self._width

    @property
    def size_bytes(self) -> int:
        """
        Return the minimum number of bytes required to store the buffer.

        Returns:
            The number of bytes needed to represent ``width`` bits.
        """
        return (self._width + 7) // 8
