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

buf.assign(0xAB, 8)
buf.toggle()         # flips all 8 bits
buf.clear()          # keeps len(buf) == 8
```

### Convert Back to Python Types

```python
buf = bitbuf.from_bytes(b"\x34\x12")

as_int = int(buf)
as_bytes = bytes(buf)
as_hex = hex(buf)

assert as_int == buf.int()
assert as_bytes == buf.bytes()
assert as_hex == buf.hex()
```

## API Reference

```python
input_types: TypeAlias = int | bytes | bytearray | memoryview | bitbuf

class bitbuf:
    def __init__(self, value: input_types = 0, width: int | None = None) -> None: ...

    @classmethod
    def from_int(cls, data: int, width: int | None = None) -> bitbuf: ...
    @classmethod
    def from_bytes(cls, data: bytes | bytearray | memoryview, width: int | None = None) -> bitbuf: ...
    @classmethod
    def zeros(cls, width: int) -> bitbuf: ...
    @classmethod
    def ones(cls, width: int) -> bitbuf: ...

    def __len__(self) -> int: ...
    def __int__(self) -> int: ...
    def __index__(self) -> int: ...
    def __bytes__(self) -> bytes: ...
    def __repr__(self) -> str: ...
    def __getitem__(self, key: int | slice) -> int: ...
    def __setitem__(self, key: int | slice, value: input_types) -> None: ...
    def __ilshift__(self, bits: int) -> bitbuf: ...
    def __irshift__(self, bits: int) -> bitbuf: ...

    def assign(self, value: input_types = 0, width: int | None = None) -> None: ...
    def resize(self, width: int) -> None: ...
    def clear(self) -> None: ...

    def get_bit(self, pos: int) -> int: ...
    def get_bits(self, pos: int, width: int) -> int: ...
    def get_bits_as_bytes(self, pos: int, width: int) -> bytes: ...
    def get_bits_as_bytearray(self, pos: int, width: int) -> bytearray: ...
    def slice(self, pos: int, width: int) -> bitbuf: ...

    def set_bit(self, pos: int, value: int = 1) -> None: ...
    def set_bits(self, pos: int, value: input_types = 0, width: int | None = None) -> None: ...
    def set_ones(self, pos: int, width: int) -> None: ...
    def set_zeros(self, pos: int, width: int) -> None: ...
    def toggle(self, pos: int = 0, width: int | None = None) -> None: ...

    def lshift(self, bits: int) -> None: ...
    def rshift(self, bits: int) -> None: ...

    def append_low(self, value: input_types = 0, width: int | None = None) -> None: ...
    def append_high(self, value: input_types = 0, width: int | None = None) -> None: ...
    def delete_low(self, width: int) -> None: ...
    def delete_high(self, width: int) -> None: ...
    def pop_low(self, width: int) -> int: ...
    def pop_high(self, width: int) -> int: ...

    def bytes(self) -> bytes: ...
    def int(self) -> int: ...
    def hex(self) -> str: ...

    @property
    def width(self) -> int: ...
    @property
    def size_bytes(self) -> int: ...
```

## Development

```bash
python -m pip install -e ".[test]"
python -m pytest
```
