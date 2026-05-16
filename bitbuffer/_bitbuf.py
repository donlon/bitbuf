from __future__ import annotations

from ._utils import ensure_int


class bitbuf:
    """A mutable little-endian bit buffer backed by Python's ``int``.

    Bits are indexed from least significant to most significant, so position
    ``0`` refers to the least significant bit of ``data``. The buffer stores an
    explicit bit ``size`` and keeps its integer payload masked to that width.

    Byte conversion uses little-endian order for both input and output.
    """

    def __init__(self, size: int = 0, data: int | bytes = 0):
        """
        Args:
            size: Initial buffer width in bits.
            data: Initial buffer contents as an integer or little-endian bytes.
        """
        self.size = int(size)
        if self.size < 0:
            raise ValueError("size must be non-negative")
        self.data = ensure_int(data)
        self._trim()

    @classmethod
    def from_int(cls, data: int, size: int | None = None) -> bitbuf:
        """Create a buffer from an integer."""
        data = ensure_int(data)
        if size is None:
            size = data.bit_length()
        return cls(size, data)

    @classmethod
    def from_bytes(cls, data: bytes | bytearray | memoryview, size: int | None = None) -> bitbuf:
        """Create a buffer from little-endian bytes."""
        if size is None:
            size = len(data) * 8
        return cls(size, data)

    @classmethod
    def zeros(cls, size: int) -> bitbuf:
        """Create a zero-filled buffer."""
        return cls(size, 0)

    @classmethod
    def ones(cls, size: int) -> bitbuf:
        """Create a one-filled buffer."""
        if size < 0:
            raise ValueError("size must be non-negative")
        return cls(size, (1 << size) - 1 if size else 0)

    def __len__(self) -> int:
        return self.size

    def __int__(self) -> int:
        return self.toint()

    def __bytes__(self) -> bytes:
        return self.tobytes()

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
            self.set_bits(start, stop - start, value)
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

    def _trim(self) -> None:
        self.data &= self._mask()

    def _slice_bounds(self, key: slice) -> tuple[int, int]:
        if key.step is not None:
            raise TypeError("slice step is not supported")
        if key.start is None or key.stop is None:
            raise TypeError("slice start and stop are required")
        start = int(key.start)
        stop = int(key.stop)
        if stop < start:
            raise ValueError("slice stop must be greater than or equal to start")
        return start, stop

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

    def get_bit(self, position: int) -> int:
        """
        Return the bit value at ``position``.

        Args:
            position: Bit index, where 0 refers to the least significant bit.
        """
        if position < 0 or position >= self.size:
            raise IndexError("bit position out of range")
        return (self.data >> position) & 1

    def set_bit(self, position: int, value: int = 1) -> None:
        """
        Set the bit at ``position`` to ``value``.

        Args:
            position: Bit index, where 0 refers to the least significant bit.
            value: Bit value to write. Non-zero values are truncated to 1.
        """
        self.set_bits(position, 1, value)

    def set_bits(self, position: int, width: int, value: int | bytes = 0) -> None:
        """
        Replace ``width`` bits starting at ``position`` with ``value``.

        The least significant bit of ``value`` is written to ``position``.

        Args:
            position: Starting bit index for the write.
            width: Number of bits to replace.
            value: Replacement value, interpreted in little-endian bit order.
        """
        if width < 0:
            raise ValueError("width must be non-negative")
        if position < 0 or position + width > self.size:
            raise IndexError("bit range out of range")
        if width == 0:
            return
        value = ensure_int(value)
        mask = ((1 << width) - 1) << position
        value_mask = (value & ((1 << width) - 1)) << position
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
        return (self.data >> position) & ((1 << width) - 1)

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
        else:
            self.data = (self.data << bits) & self._mask()

    def rshift(self, bits: int) -> None:
        """
        Shift the buffer right by ``bits`` while keeping the width unchanged.

        Args:
            bits: Number of bit positions to shift toward the least significant end.
        """
        if bits < 0:
            raise ValueError("bits must be non-negative")
        self.data >>= bits

    def lsplice(self, width: int, value: int | bytes = 0) -> None:
        """
        Append ``width`` bits to the most significant side and grow the buffer.

        Args:
            width: Number of bits to append.
            value: Bits to append, interpreted in little-endian bit order.
        """
        if width < 0:
            raise ValueError("width must be non-negative")
        value = ensure_int(value) & ((1 << width) - 1 if width > 0 else 0)
        self.data |= value << self.size
        self.size += width

    def rsplice(self, width: int, value: int | bytes = 0) -> None:
        """
        Append ``width`` bits to the least significant side and grow the buffer.

        Args:
            width: Number of bits to append.
            value: Bits to append, interpreted in little-endian bit order.
        """
        if width < 0:
            raise ValueError("width must be non-negative")
        value = ensure_int(value) & ((1 << width) - 1 if width > 0 else 0)
        self.data = (self.data << width) | value
        self.size += width

    def append_msb(self, width: int, value: int | bytes = 0) -> None:
        """Alias for ``lsplice``."""
        self.lsplice(width, value)

    def append_lsb(self, width: int, value: int | bytes = 0) -> None:
        """Alias for ``rsplice``."""
        self.rsplice(width, value)

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
        self.data >>= width
        self.size -= width
        return value

    def tobytes(self) -> bytes:
        """
        Return the buffer contents as little-endian bytes.

        Returns:
            The current buffer value serialized to the minimum number of bytes.
        """
        return self.data.to_bytes(self.size_bytes(), "little")

    def toint(self) -> int:
        """
        Return the buffer contents as an integer.

        Returns:
            The current buffer value as a Python integer.
        """
        return self.data

    def size_bytes(self) -> int:
        """
        Return the minimum number of bytes required to store the buffer.

        Returns:
            The number of bytes needed to represent ``size`` bits.
        """
        return (self.size + 7) // 8
