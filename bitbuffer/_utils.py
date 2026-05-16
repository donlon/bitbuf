def ensure_int(value: int | bytes | bytearray | memoryview) -> int:
    """Return *value* as an integer using little-endian byte order."""
    if isinstance(value, int):
        return value
    if isinstance(value, (bytes, bytearray, memoryview)):
        return int.from_bytes(value, "little")
    raise TypeError("value must be an int or bytes-like object")
