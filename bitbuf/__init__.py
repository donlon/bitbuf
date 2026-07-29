# SPDX-License-Identifier: MIT

"""Lightweight and efficient bit-level buffer mutation library for Python. Implemented in C++ with
Cython support."""

from ._bitbuf import _ABI_VERSION as ABI_VERSION, bitbuf


__INCLUDE_DIR = None
__SOURCE_DIR = None


def check_abi_version(
    *,
    abi_version: int = -1,
    library_name: str = "",
):
    """Check if ABI version of Cython interface of the built library matches with ``abi_version``.
    
    Raises:
        ImportError: if ABI version mismatch with the given version
    """
    if abi_version != ABI_VERSION:
        lib_name = library_name + " " if library_name else ""
        raise ImportError(
            f"Library {lib_name}requires bitbuf with abi version {abi_version} "
            f"but installed has version {ABI_VERSION}. "
            "Please update either of these packages."
        )


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
        __SOURCE_DIR = _current_dir / "src"


def get_include_dir() -> str:
    """Return include directory contains bitbuf header files, which is required to build Cython
    module that depends on bitbuf module."""
    if __INCLUDE_DIR is None:
        __init_paths()
    return str(__INCLUDE_DIR)


def get_sources() -> list[str]:
    """Return list of C++ source files, which are required to build Cython module that depends on
    bitbuf module."""
    if __SOURCE_DIR is None:
        __init_paths()
    return [str(__SOURCE_DIR / "bitbuf.cpp")]


__all__ = (
    "ABI_VERSION",
    "bitbuf",
    "check_abi_version",
    "get_include_dir",
    "get_sources",
)
