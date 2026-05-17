from __future__ import annotations

from ._utils import ensure_int


class bitbuf:
    """A mutable little-endian bit buffer backed by Python's ``int``.

    Bits are indexed from least significant to most significant, so position
    ``0`` refers to the least significant bit of ``data``. The buffer stores an
    explicit bit ``size`` and keeps its integer payload masked to that width.

    Byte conversion uses little-endian order for both input and output.
    """

    def __init__(self, value: int | bytes | bitbuf = 0, size: int | None = None):
        """
        Args:
            value: Initial buffer contents as an integer, little-endian bytes,
                or another bitbuf.
            size: Initial buffer width in bits. Ignored when ``value`` is a bitbuf.
        """
        data, self.size = self._sized_value(value, size)
        self._offset = 0
        self.data = data
        self._trim()

    @classmethod
    def from_int(cls, data: int, size: int | None = None) -> bitbuf:
        """Create a buffer from an integer."""
        data = ensure_int(data)
        if size is None:
            size = data.bit_length()
        return cls(data, size)

    @classmethod
    def from_bytes(cls, data: bytes | bytearray | memoryview, size: int | None = None) -> bitbuf:
        """Create a buffer from little-endian bytes."""
        if size is None:
            size = len(data) * 8
        return cls(data, size)

    @classmethod
    def zeros(cls, size: int) -> bitbuf:
        """Create a zero-filled buffer."""
        return cls(0, size)

    @classmethod
    def ones(cls, size: int) -> bitbuf:
        """Create a one-filled buffer."""
        if size < 0:
            raise ValueError("size must be non-negative")
        return cls((1 << size) - 1 if size else 0, size)

    def __len__(self) -> int:
        return self.size

    def __int__(self) -> int:
        return self.toint()

    def __index__(self) -> int:
        return self.toint()

    def __bytes__(self) -> bytes:
        return self.tobytes()

    def __repr__(self) -> str:
        return f"bitbuf(len={self.size}, hex={self._repr_hex()})"

    def __getitem__(self, key: int | slice) -> int:
        if isinstance(key, slice):
            start, stop = self._slice_bounds(key)
            return self.get_bits(start, stop - start)
        if isinstance(key, int):
            return self.get_bit(key)
        raise TypeError("bit index must be an int or slice")

    def __setitem__(self, key: int | slice, value: int | bytes) -> None:
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
        if self.size <= 0:
            return 0
        return (1 << self.size) - 1

    @staticmethod
    def _sized_value(value: int | bytes | bitbuf, size: int | None) -> tuple[int, int]:
        if isinstance(value, bitbuf):
            return value.toint(), len(value)
        if isinstance(value, (bytes, bytearray, memoryview)):
            data = ensure_int(value)
            if size is None:
                size = len(value) * 8
        else:
            data = ensure_int(value)
            if size is None:
                size = data.bit_length()
        size = int(size)
        if size < 0:
            raise ValueError("size must be non-negative")
        return data, size

    def _trim(self) -> None:
        if self.size == 0:
            self.data = 0
            self._offset = 0
            return
        self.data &= self._mask() << self._offset

    def _increase_offset(self, bits: int) -> None:
        self._offset += bits
        if self._offset >= 32:
            words = self._offset // 32
            self.data >>= words * 32
            self._offset %= 32

    def _decrease_offset(self, bits: int) -> None:
        if bits <= self._offset:
            self._offset -= bits
            return
        words = (bits - self._offset + 31) // 32
        self.data <<= words * 32
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

    def _normalize_position(self, position: int, allow_end: bool = False) -> int:
        if position < 0:
            position += self.size
        upper = self.size if allow_end else self.size - 1
        if position < 0 or position > upper:
            raise IndexError("bit position out of range")
        return position

    def _repr_hex(self) -> str:
        hex_digits = max(1, (self.size + 3) // 4)
        if hex_digits <= 64:
            return self.hex()
        low = self.get_bits(0, 64)
        high_width = min(64, self.size)
        high = self.get_bits(self.size - high_width, high_width)
        return f"0x{high:x}...{low:016x}"

    def resize(self, size: int) -> None:
        """
        Resize the buffer to ``size`` bits.

        When the new size is smaller than the current size, bits beyond the new
        most significant bit are discarded.

        Args:
            size: Target buffer width in bits.
        """
        self.size = int(size)
        if self.size < 0:
            raise ValueError("size must be non-negative")
        self._trim()

    def assign(self, value: int | bytes | bitbuf = 0, size: int | None = None) -> None:
        """
        Replace the whole buffer with ``value`` and ``size``.

        Args:
            value: Replacement contents as an integer, little-endian bytes, or
                another bitbuf.
            size: Replacement width in bits. Ignored when ``value`` is a bitbuf.
        """
        data, self.size = self._sized_value(value, size)
        self._offset = 0
        self.data = data
        self._trim()

    def clear(self) -> None:
        """Clear all bits while keeping the current size unchanged."""
        self.data = 0
        self._offset = 0

    def get_bit(self, position: int) -> int:
        """
        Return the bit value at ``position``.

        Args:
            position: Bit index, where 0 refers to the least significant bit.
        """
        position = self._normalize_position(position)
        return (self.data >> (self._offset + position)) & 1

    def set_bit(self, position: int, value: int = 1) -> None:
        """
        Set the bit at ``position`` to ``value``.

        Args:
            position: Bit index, where 0 refers to the least significant bit.
            value: Bit value to write. Non-zero values are truncated to 1.
        """
        self.set_bits(self._normalize_position(position), value, 1)

    def set_bits(self, position: int, value: int | bytes | bitbuf = 0, size: int | None = None) -> None:
        """
        Replace ``size`` bits starting at ``position`` with ``value``.

        The least significant bit of ``value`` is written to ``position``.

        Args:
            position: Starting bit index for the write.
            value: Replacement value, interpreted in little-endian bit order.
            size: Number of bits to replace. Ignored when ``value`` is a bitbuf.
        """
        value, size = self._sized_value(value, size)
        if position < 0 or position + size > self.size:
            raise IndexError("bit range out of range")
        if size == 0:
            return
        position += self._offset
        mask = ((1 << size) - 1) << position
        value_mask = (value & ((1 << size) - 1)) << position
        self.data = (self.data & ~mask) | value_mask

    def get_bits(self, position: int, width: int) -> int:
        """
        Return ``width`` bits starting at ``position`` as an integer.

        Args:
            position: Starting bit index for the read.
            width: Number of bits to read.
        """
        if width < 0:
            raise ValueError("width must be non-negative")
        if position < 0 or position + width > self.size:
            raise IndexError("bit range out of range")
        if width == 0:
            return 0
        return (self.data >> (self._offset + position)) & ((1 << width) - 1)

    def get_bits_as_buf(self, position: int, width: int) -> bitbuf:
        """
        Return ``width`` bits starting at ``position`` as a new bitbuf.
        """
        return bitbuf(self.get_bits(position, width), width)

    def get_bits_as_bytes(self, position: int, width: int) -> bytes:
        """
        Return ``width`` bits starting at ``position`` as little-endian bytes.
        """
        return self.get_bits(position, width).to_bytes((width + 7) // 8, "little")

    def set_ones(self, position: int, width: int) -> None:
        """
        Set ``width`` bits starting at ``position`` to 1.

        Args:
            position: Starting bit index for the write.
            width: Number of bits to set.
        """
        if width < 0:
            raise ValueError("width must be non-negative")
        if position < 0 or position + width > self.size:
            raise IndexError("bit range out of range")
        if width == 0:
            return
        self.data |= ((1 << width) - 1) << (self._offset + position)

    def set_zeros(self, position: int, width: int) -> None:
        """
        Set ``width`` bits starting at ``position`` to 0.

        Args:
            position: Starting bit index for the write.
            width: Number of bits to clear.
        """
        if width < 0:
            raise ValueError("width must be non-negative")
        if position < 0 or position + width > self.size:
            raise IndexError("bit range out of range")
        if width == 0:
            return
        self.data &= ~(((1 << width) - 1) << (self._offset + position))

    def lshift(self, bits: int) -> None:
        """
        Shift the buffer left by ``bits`` while keeping the width unchanged.

        Args:
            bits: Number of bit positions to shift toward the most significant end.
        """
        if bits < 0:
            raise ValueError("bits must be non-negative")
        if bits >= self.size:
            self.data = 0
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
        if bits >= self.size:
            self.data = 0
            self._offset = 0
        else:
            self._increase_offset(bits)
            self._trim()

    def append_msb(self, value: int | bytes | bitbuf = 0, size: int | None = None) -> None:
        """
        Append ``size`` bits to the most significant side and grow the buffer.

        Args:
            value: Bits to append, interpreted in little-endian bit order.
            size: Number of bits to append. Ignored when ``value`` is a bitbuf.
        """
        value, size = self._sized_value(value, size)
        value &= ((1 << size) - 1 if size > 0 else 0)
        self.data |= value << (self._offset + self.size)
        self.size += size

    def append_lsb(self, value: int | bytes | bitbuf = 0, size: int | None = None) -> None:
        """
        Append ``size`` bits to the least significant side and grow the buffer.

        Args:
            value: Bits to append, interpreted in little-endian bit order.
            size: Number of bits to append. Ignored when ``value`` is a bitbuf.
        """
        value, size = self._sized_value(value, size)
        if size == 0:
            return
        value &= (1 << size) - 1
        self._decrease_offset(size)
        self.size += size
        self.set_bits(0, value, size)
        self._trim()

    def delete_msb(self, width: int) -> int:
        """Remove and return ``width`` bits from the most significant side."""
        if width < 0:
            raise ValueError("width must be non-negative")
        if width > self.size:
            raise IndexError("bit range out of range")
        if width == 0:
            return 0
        position = self.size - width
        value = self.get_bits(position, width)
        self.size -= width
        self._trim()
        return value

    def delete_lsb(self, width: int) -> int:
        """Remove and return ``width`` bits from the least significant side."""
        if width < 0:
            raise ValueError("width must be non-negative")
        if width > self.size:
            raise IndexError("bit range out of range")
        if width == 0:
            return 0
        value = self.get_bits(0, width)
        self.size -= width
        self._increase_offset(width)
        self._trim()
        return value

    def tobytes(self) -> bytes:
        """
        Return the buffer contents as little-endian bytes.

        Returns:
            The current buffer value serialized to the minimum number of bytes.
        """
        return self.toint().to_bytes(self.size_bytes(), "little")

    def toint(self) -> int:
        """
        Return the buffer contents as an integer.

        Returns:
            The current buffer value as a Python integer.
        """
        return (self.data >> self._offset) & self._mask()

    def hex(self) -> str:
        """Return ``hex(self.toint())``."""
        return hex(self.toint())

    def size_bytes(self) -> int:
        """
        Return the minimum number of bytes required to store the buffer.

        Returns:
            The number of bytes needed to represent ``size`` bits.
        """
        return (self.size + 7) // 8
