# SPDX-License-Identifier: MIT
"""Build helpers for compiling Cython/C++ extensions against bitbuf.

Downstream extensions that ``cimport`` the ``bitbuf`` package need the C++
header on their include path and the BitBuf implementation compiled in::

    import bitbuf.cython
    from setuptools import Extension

    ext = Extension(
        "mymod",
        sources=["mymod.pyx", *bitbuf.cython.get_source()],
        include_dirs=[bitbuf.cython.get_include()],
        language="c++",
    )

Both functions work from an installed wheel (headers/sources shipped inside the
package) and from an in-tree checkout (they live in ``cpp/``).
"""

import os
from pathlib import Path


_current_dir = Path(__file__).parent
_root_dir = _current_dir / ".."
_is_dev = os.path.exists(_root_dir / "pyproject.toml")

if _is_dev:
    _INCLUDE_DIR = _root_dir / "cpp"
    _SOURCE_DIR = _root_dir / "cpp"
else:
    _INCLUDE_DIR = _current_dir / "include"
    _SOURCE_DIR = _current_dir / "cpp"


def get_include() -> str:
    """Return the directory containing ``bitbuf.h`` (pass to ``include_dirs``)."""
    return str(_INCLUDE_DIR)


def get_source() -> list[str]:
    """Return the C++ source files to compile into a consumer extension.

    Currently ``[<path>/bitbuf.cpp]`` -- the standalone C++ implementation
    (no Python dependency). Add these to your extension's ``sources``.
    """
    return [str(_SOURCE_DIR / "bitbuf.cpp")]


__all__ = ["get_include", "get_source"]
