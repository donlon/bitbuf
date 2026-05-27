# bitbuf

A fast and lightweight bit-level mutation library for Python.

> Notice: `bitbuf` is currently a beta proof of concept. The public API is
> still settling, and performance is not fully optimized yet. A future version
> is expected to optimize the internal implementation with a native programming
> language backend.

## Installation

```bash
pip install bitbuf
```

## Quick Start

```python
from bitbuf import bitbuf

buf = bitbuf.from_bytes(b"\x34\x12")

buf[4:12] = 0xAB
buf <<= 3
buf.append_msb(0b1010, 4)

value = int(buf)
payload = bytes(buf)
```

Bit position `0` is the least significant bit. Byte conversion always uses
little-endian order.

## Examples

### Create Buffers

```python
from bitbuf import bitbuf

empty = bitbuf()
fixed = bitbuf(0b1010_0101, 8)
from_int = bitbuf.from_int(0x1234, size=16)
from_bytes = bitbuf.from_bytes(b"\x34\x12")
zeros = bitbuf.zeros(16)
ones = bitbuf.ones(8)
```

### Read and Write Bits

```python
buf = bitbuf(0b1010_0101, 8)

lowest = buf[0]      # 1
highest = buf[-1]    # 1
buf[1] = 1           # set bit 1
buf.set_bit(7, 0)    # clear bit 7
```

### Read and Write Bit Ranges

Slices use `[start:stop]`, where `start` is the LSB-first bit position and
`stop - start` is the width.

```python
buf = bitbuf(0x1234, 16)

field = buf[4:12]
field_bytes = buf.get_bits_as_bytes(4, 8)
field_bytearray = buf.get_bits_as_bytearray(4, 8)
field_slice = buf.slice(4, 8) # return a slice as bitbuf
buf[4:12] = 0xAB

same_field = buf.get_bits(4, 8)
buf.set_bits(0, 0b1111, 4)
```

### Set or Clear Ranges

```python
buf = bitbuf(0, 8)

buf.set_ones(2, 4)   # 0b0011_1100
buf.set_zeros(3, 2)  # 0b0010_0100
```

### Shift In Place

All operations mutate the current buffer.

```python
buf = bitbuf(0b0000_1111, 8)

buf <<= 2            # same as buf.lshift(2)
buf >>= 1            # same as buf.rshift(1)
```

### Append and Delete Bits

```python
buf = bitbuf(0b0011, 4)

buf.append_high(0b101, 3)  # 0b101_0011
buf.append_low(0b10, 2)    # 0b1010011_10

buf.delete_low(2)           # discard low bits
high = buf.pop_high(3)      # remove and return high bits
```

### Replace or Clear Contents

```python
buf = bitbuf(0x1234, 16)

buf.assign(0xAB, 8).toggle() # assign and then flips all 8 bits
buf.clear()          # keeps len(buf) == 8
```

### Convert Back to Python Types

```python
buf = bitbuf.from_bytes(b"\x34\x12")

as_int = int(buf)
as_bytes = bytes(buf)
as_bytearray = buf.bytearray()
as_hex = hex(buf)

assert as_int == buf.int()
assert as_bytes == buf.bytes()
assert as_bytearray == bytearray(as_bytes)
assert as_hex == buf.hex()
```

## API Reference

```python
input_types: TypeAlias = int | bytes | bytearray | memoryview | bitbuf

class bitbuf:
    def __init__(self, value: input_types = 0, width: int | None = None): ...
    # Create a mutable LSB-first buffer from an int, bytes-like object, or bitbuf.

    @classmethod
    def from_int(cls, data: int, width: int | None = None) -> bitbuf: ...
    # Build a buffer from an integer, inferring width from bit_length when omitted.
    @classmethod
    def from_bytes(cls, data: bytes | bytearray | memoryview, width: int | None = None) -> bitbuf: ...
    # Build a buffer from little-endian bytes-like data.
    @classmethod
    def zeros(cls, width: int) -> bitbuf: ...
    # Build a zero-filled buffer with the requested width.
    @classmethod
    def ones(cls, width: int) -> bitbuf: ...
    # Build a one-filled buffer with the requested width.

    def __len__(self) -> int: ...
    # Return width in bits.
    def __int__(self) -> int: ...
    # Convert the buffer to an integer.
    def __index__(self) -> int: ...
    # Allow numeric helpers such as hex(buf).
    def __bytes__(self) -> bytes: ...
    # Convert the buffer to little-endian bytes.
    def __repr__(self) -> str: ...
    # Return a concise debug representation with width and hex data.
    def __getitem__(self, key: int | slice) -> int: ...
    # Read one bit or an integer-valued bit range.
    def __setitem__(self, key: int | slice, value: input_types) -> None: ...
    # Write one bit or replace a bit range.
    def __ilshift__(self, bits: int) -> Self: ...
    # Shift left in place.
    def __irshift__(self, bits: int) -> Self: ...
    # Shift right in place.

    def assign(self, value: input_types = 0, width: int | None = None) -> Self: ...
    # Replace the entire buffer contents and width.
    def resize(self, width: int) -> Self: ...
    # Change width, trimming discarded high bits when shrinking.
    def clear(self) -> Self: ...
    # Zero all bits while keeping the current width.

    def get_bit(self, pos: int) -> int: ...
    # Return one bit at the given LSB-first position.
    def get_bits(self, pos: int, width: int) -> int: ...
    # Return a bit range as an integer.
    def get_bits_as_bytes(self, pos: int, width: int) -> bytes: ...
    # Return a bit range as little-endian bytes.
    def get_bits_as_bytearray(self, pos: int, width: int) -> bytearray: ...
    # Return a bit range as little-endian bytearray.
    def slice(self, pos: int, width: int) -> bitbuf: ...
    # Return a bit range as a new bitbuf.

    def set_bit(self, pos: int, value: int = 1) -> Self: ...
    # Set or clear one bit.
    def set_bits(self, pos: int, value: input_types = 0, width: int | None = None) -> Self: ...
    # Replace a bit range with a sized value.
    def set_ones(self, pos: int, width: int) -> Self: ...
    # Fill a bit range with ones.
    def set_zeros(self, pos: int, width: int) -> Self: ...
    # Fill a bit range with zeros.
    def toggle(self, pos: int = 0, width: int | None = None) -> Self: ...
    # Flip all bits from pos upward, or a specific range when width is given.

    def lshift(self, bits: int) -> Self: ...
    # Shift bits toward the high side while preserving width.
    def rshift(self, bits: int) -> Self: ...
    # Shift bits toward the low side while preserving width.

    def append_low(self, value: input_types = 0, width: int | None = None) -> Self: ...
    # Grow the buffer by appending bits on the low side.
    def append_high(self, value: input_types = 0, width: int | None = None) -> Self: ...
    # Grow the buffer by appending bits on the high side.
    def delete_low(self, width: int) -> Self: ...
    # Discard low-side bits without returning them.
    def delete_high(self, width: int) -> Self: ...
    # Discard high-side bits without returning them.
    def pop_low(self, width: int) -> int: ...
    # Remove and return low-side bits.
    def pop_high(self, width: int) -> int: ...
    # Remove and return high-side bits.

    def bytearray(self) -> bytearray: ...
    # Return the whole buffer as little-endian bytearray.
    def bytes(self) -> bytes: ...
    # Return the whole buffer as little-endian bytes.
    def hex(self) -> str: ...
    # Return hex(self.int()).
    def int(self) -> int: ...
    # Return the whole buffer as an integer.

    @property
    def width(self) -> int: ...
    # Width of the buffer in bits.
    @property
    def size_bytes(self) -> int: ...
    # Minimum byte count needed to store the buffer width.
```

## Development

```bash
python -m pip install -e ".[test]"
python -m pytest
```
