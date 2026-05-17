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
field_buf = buf.get_bits_as_buf(4, 8)
field_bytes = buf.get_bits_as_bytes(4, 8)
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

buf.append_msb(0b101, 3)  # 0b101_0011
buf.append_lsb(0b10, 2)   # 0b1010011_10

low = buf.delete_lsb(2)
high = buf.delete_msb(3)
```

### Replace or Clear Contents

```python
buf = bitbuf(0x1234, 16)

buf.assign(0xAB, 8)
buf.clear()          # keeps len(buf) == 8
```

### Convert Back to Python Types

```python
buf = bitbuf.from_bytes(b"\x34\x12")

as_int = int(buf)
as_bytes = bytes(buf)
as_hex = hex(buf)

assert as_int == buf.toint()
assert as_bytes == buf.tobytes()
assert as_hex == buf.hex()
```

## Development

```bash
python -m pip install -e ".[test]"
python -m pytest
```
