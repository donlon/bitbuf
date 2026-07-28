# SPDX-License-Identifier: MIT

"""Lightweight and efficient bit-level buffer mutation library for Python. Implemented in C++ with
Cython support."""

from ._bitbuf import bitbuf

__INCLUDE_DIR = None
__SOURCE_DIR = None


def __init_paths():
    global __INCLUDE_DIR, __SOURCE_DIR

    import os
    from pathlib import Path

    _current_dir = Path(__file__).parent
    _root_dir = _current_dir / ".."
    _is_dev = os.path.exists(_root_dir / "pyproject.toml")

    if _is_dev:
        __INCLUDE_DIR = _root_dir / "cpp"
        __SOURCE_DIR = _root_dir / "cpp"
    else:
        __INCLUDE_DIR = _current_dir / "include"
        __SOURCE_DIR = _current_dir / "cpp"


def get_bitbuf_include_dir() -> str:
    """Return include directory contains bitbuf header files, which is required to build Cython
    module that depends on bitbuf module."""
    if __INCLUDE_DIR is None:
        __init_paths()
    return str(__INCLUDE_DIR)


def get_bitbuf_sources() -> list[str]:
    """Return list of C++ source files, which are required to build Cython module that depends on
    bitbuf module."""
    if __SOURCE_DIR is None:
        __init_paths()
    return [str(__SOURCE_DIR / "bitbuf.cpp")]


__all__ = ("bitbuf", "get_bitbuf_include_dir", "get_bitbuf_sources")
