# SPDX-License-Identifier: MIT
"""Let pytest collect tests written directly in Cython (``test_*.pyx``).

Any ``test_*.pyx`` next to this file is cythonized and compiled in place (as a
C++ extension that links cpp/bitbuf.cpp), then its ``test_*`` functions are
collected like a normal test module. Collection is skipped cleanly if Cython,
setuptools, the built ``bitbuf`` extension, or a C++ compiler is unavailable.
"""

import importlib.util
import pathlib
import sys
import sysconfig
from contextlib import nullcontext
from typing_extensions import override

import pytest

TESTS_DIR = pathlib.Path(__file__).resolve().parent
REPO_ROOT = TESTS_DIR.parent
BUILD_DIR = TESTS_DIR / "cython-build"

_built_modules = {}


def _build_pyx(pyx_path: pathlib.Path):
    key = str(pyx_path)
    if key in _built_modules:
        return _built_modules[key]

    from bitbuf import get_include_dir, get_sources
    from Cython.Build import cythonize
    from setuptools import Extension
    from setuptools.dist import Distribution

    name = pyx_path.stem
    if sys.platform == "win32":
        cxx_args = [
            "/std:c++17",
            "/EHsc",
            "/wd4146",  # disable: unary minus operator applied to unsigned type
            "/wd4551",  # disable: function call missing argument list (Cython should fix it)
        ]
    else:
        cxx_args = ["-std=c++17"]

    ext = Extension(
        name,
        include_dirs=[get_include_dir()],
        sources=[str(pyx_path), *get_sources()],
        language="c++",
        extra_compile_args=cxx_args,
        define_macros=[("NOMINMAX", None)],
    )

    BUILD_DIR.mkdir(parents=True, exist_ok=True)
    ext_modules = cythonize(
        [ext],
        include_path=[str(REPO_ROOT)],  # so `cimport bitbuf` resolves in-tree
        language_level=3,
        build_dir=str(BUILD_DIR / "cythonized"),
    )

    dist = Distribution({"name": name, "ext_modules": ext_modules})
    cmd = dist.get_command_obj("build_ext")
    cmd.build_lib = str(TESTS_DIR)  # final .pyd/.so lands next to the .pyx (in place)
    cmd.build_temp = str(BUILD_DIR / "temp")
    cmd.inplace = 0
    cmd.ensure_finalized()

    # WTF???
    ctx = nullcontext()
    if sys.platform == "win32":
        # setuptools probes MSVC via subprocess; under some Windows terminals,
        # inherited stdin can be invalid and crash with WinError 6.
        from setuptools._distutils.compilers.C import msvc as _msvc

        orig_check_output = _msvc.subprocess.check_output

        def _safe_check_output(*args, **kwargs):
            kwargs.setdefault("stdin", _msvc.subprocess.DEVNULL)
            return orig_check_output(*args, **kwargs)

        class _PatchCheckOutput:
            def __enter__(self):
                _msvc.subprocess.check_output = _safe_check_output

            def __exit__(self, exc_type, exc, tb):
                _msvc.subprocess.check_output = orig_check_output

        ctx = _PatchCheckOutput()

    with ctx:
        cmd.run()

    suffix = sysconfig.get_config_var("EXT_SUFFIX")
    so_path = TESTS_DIR / (name + suffix)
    spec = importlib.util.spec_from_file_location(name, so_path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    _built_modules[key] = module
    return module


class CythonModule(pytest.Module):
    @override
    def _getobj(self):
        try:
            import Cython  # noqa: F401
            import setuptools  # noqa: F401
        except Exception as exc:
            pytest.skip(f"Cython interface tests need Cython + built bitbuf ({exc})")
        try:
            return _build_pyx(self.path)
        finally:
            pass

    @override
    def collect(self):
        module = self.obj
        for name in sorted(dir(module)):
            if not name.startswith("test_"):
                continue
            obj = getattr(module, name)
            if callable(obj):
                # callobj bypasses pytest's inspect.isfunction check, which does
                # not recognise compiled Cython functions.
                yield pytest.Function.from_parent(self, name=name, callobj=obj)


def pytest_collect_file(parent, file_path):
    if file_path.suffix == ".pyx" and file_path.name.startswith("test_"):
        return CythonModule.from_parent(parent, path=file_path)
