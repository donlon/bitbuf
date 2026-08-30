# SPDX-License-Identifier: MIT

# import pytest
import pickle

from bitbuf import bitbuf


def test_pickle_roundtrip_empty_object():
    original = bitbuf()

    restored = pickle.loads(pickle.dumps(original))

    assert isinstance(restored, bitbuf)
    assert restored is not original
    assert original == bitbuf()
    assert restored.length == original.length
    assert int(restored) == int(original)


def test_pickle_roundtrip_preserves_value_and_width():
    original = bitbuf(0xABC, 12)

    restored = pickle.loads(pickle.dumps(original))

    assert isinstance(restored, bitbuf)
    assert restored is not original
    assert original == bitbuf(0xABC, 12)
    assert restored.length == original.length
    assert int(restored) == int(original)


def test_pickle_roundtrip_preserves_non_byte_aligned_values():
    original = bitbuf(0x1234_5678, 29)

    restored = pickle.loads(pickle.dumps(original, protocol=pickle.HIGHEST_PROTOCOL))

    assert original == bitbuf(0x1234_5678, 29)
    assert restored.length == 29
    assert int(restored) == int(original)
