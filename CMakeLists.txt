cmake_minimum_required(VERSION 3.18)

project(bitbuf LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
FetchContent_Declare(
    nanobind
    GIT_REPOSITORY https://github.com/wjakob/nanobind.git
    GIT_TAG v2.12.0
)
FetchContent_MakeAvailable(nanobind)

find_package(Python 3.10 REQUIRED COMPONENTS Interpreter Development.Module)
#find_package(nanobind CONFIG REQUIRED)

execute_process(
    COMMAND "${Python_EXECUTABLE}" -m nanobind --cmake_dir
    OUTPUT_VARIABLE nanobind_ROOT
    RESULT_VARIABLE nanobind_query_result
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

nanobind_add_module(
    _bitbuf
    STABLE_ABI
    NB_STATIC
    cpp/bitbuf.cpp
    cpp/py_bitbuf.cpp
    cpp/module.cpp
)

target_include_directories(_bitbuf PRIVATE cpp)
target_compile_definitions(_bitbuf PRIVATE NOMINMAX)

install(
    TARGETS _bitbuf
    LIBRARY DESTINATION bitbuf
    RUNTIME DESTINATION bitbuf
)
