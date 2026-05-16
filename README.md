# bitbuf

A small mutable, LSB-first bit buffer for Python.

```python
from bitbuf import bitbuf

buf = bitbuf.from_bytes(b"\x34\x12")

buf[4:12] = 0xAB
buf <<= 3
buf.append_msb(4, 0b1010)

value = int(buf)
payload = bytes(buf)
```

Bit position `0` is the least significant bit. Byte conversion uses
little-endian order.

## Development

```bash
python -m pip install -e ".[test]"
python -m pytest
```
