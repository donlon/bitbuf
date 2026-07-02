cmake_minimum_required(VERSION 3.18)

project(bitbuf LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

if (MSVC)
    set(BITBUF_WARNING_FLAGS
        /W4
        /permissive-
        /wd4146 # disable: unary minus operator applied to unsigned type
    )
    set(BITBUF_NO_WARNING_FLAGS /w)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    set(BITBUF_WARNING_FLAGS
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wsign-conversion
        -Wshadow
        -Wno-missing-field-initializers
        -Wno-parentheses
    )
    set(BITBUF_NO_WARNING_FLAGS -w)
else ()
    set(BITBUF_WARNING_FLAGS "")
    set(BITBUF_NO_WARNING_FLAGS "")
endif ()

function(bitbuf_enable_warnings target_name)
    target_compile_options(${target_name} PRIVATE ${BITBUF_WARNING_FLAGS})
endfunction()

find_package(Python 3.10 REQUIRED COMPONENTS Interpreter Development.Module)

# Generate the C++ source for the bitbuf._bitbuf extension from Cython.
set(_bitbuf_pyx "${CMAKE_CURRENT_SOURCE_DIR}/bitbuf/_bitbuf.pyx")
set(_bitbuf_cpp "${CMAKE_CURRENT_BINARY_DIR}/_bitbuf.cpp")
add_custom_command(
    OUTPUT "${_bitbuf_cpp}"
    COMMAND ${Python_EXECUTABLE} -m cython --cplus -3
            "${_bitbuf_pyx}" -o "${_bitbuf_cpp}"
            -I "${CMAKE_CURRENT_SOURCE_DIR}"
    DEPENDS
        "${_bitbuf_pyx}"
        "${CMAKE_CURRENT_SOURCE_DIR}/bitbuf/_bitbuf.pxd"
        "${CMAKE_CURRENT_SOURCE_DIR}/bitbuf/__init__.pxd"
        "${CMAKE_CURRENT_SOURCE_DIR}/cpp/bitbuf.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/cpp/utils.h"
    VERBATIM
    COMMENT "Cythonizing bitbuf/_bitbuf.pyx"
)

Python_add_library(
    _bitbuf
    MODULE
    WITH_SOABI
    "${_bitbuf_cpp}"
    cpp/bitbuf.cpp
    cpp/utils.cpp
)

# On Windows multi-config generators, prevent Debug from selecting *_d Python libs.
set_property(TARGET _bitbuf PROPERTY MAP_IMPORTED_CONFIG_DEBUG Release)
set_property(TARGET _bitbuf PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
set_target_properties(_bitbuf PROPERTIES PREFIX "")

target_include_directories(_bitbuf PRIVATE cpp)
target_compile_definitions(_bitbuf PRIVATE NOMINMAX Py_NO_LINK_LIB)
target_compile_options(_bitbuf PRIVATE $<$<CONFIG:Debug>:/U_DEBUG>)
target_link_libraries(_bitbuf PRIVATE Python::Module)

# Enable warnings on the hand-written sources but not on the Cython-generated
# _bitbuf.cpp (which is noisy and not ours to fix).
set_source_files_properties(cpp/bitbuf.cpp cpp/utils.cpp
    TARGET_DIRECTORY _bitbuf
    PROPERTIES COMPILE_OPTIONS "${BITBUF_WARNING_FLAGS}")
set_source_files_properties("${_bitbuf_cpp}"
    TARGET_DIRECTORY _bitbuf
    PROPERTIES COMPILE_OPTIONS "${BITBUF_NO_WARNING_FLAGS}")

add_custom_command(
    TARGET _bitbuf
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    $<TARGET_FILE:_bitbuf>
    "${CMAKE_CURRENT_SOURCE_DIR}/bitbuf/"
    VERBATIM
    COMMENT "Copying output Python library to the source directory..."
)

install(
    TARGETS _bitbuf
    LIBRARY DESTINATION bitbuf
    RUNTIME DESTINATION bitbuf
)

# Ship the header, the BitBuf implementation, and the Cython declarations so
# downstream Cython/C extensions can build against the underlying BitBuf class.
# The header goes under bitbuf/include, the compilable source under bitbuf/src,
# and the package .pxd files sit in the package root.
install(
    FILES cpp/bitbuf.h
    DESTINATION bitbuf/include
)
install(
    FILES cpp/bitbuf.cpp
    DESTINATION bitbuf/src
)
install(
    FILES bitbuf/__init__.pxd bitbuf/_bitbuf.pxd
    DESTINATION bitbuf
)

option(BITBUF_BUILD_CPP_TESTS "Build C++ unit tests for BitBuf" OFF)
option(BITBUF_SANITIZE "Build the C++ tests with AddressSanitizer" ON)

if (BITBUF_BUILD_CPP_TESTS)
    enable_testing()
    FetchContent_Declare(
        doctest
        GIT_REPOSITORY https://github.com/doctest/doctest.git
        GIT_TAG v2.5.2
    )
    FetchContent_MakeAvailable(doctest)

    file(GLOB BITBUF_TEST_SOURCES CONFIGURE_DEPENDS
        cpp/tests/bitbuf/test_*.cpp
    )

    add_executable(bitbuf_cpp_tests
        cpp/bitbuf.cpp
        ${BITBUF_TEST_SOURCES}
    )

    target_include_directories(bitbuf_cpp_tests PRIVATE cpp)
    target_compile_definitions(bitbuf_cpp_tests PRIVATE NOMINMAX)
    target_compile_definitions(bitbuf_cpp_tests PRIVATE BITBUF_TESTS)
    target_link_libraries(bitbuf_cpp_tests PRIVATE doctest::doctest)
    bitbuf_enable_warnings(bitbuf_cpp_tests)

    if (BITBUF_SANITIZE)
        if (MSVC)
            target_compile_options(bitbuf_cpp_tests PRIVATE /fsanitize=address)
            # The MSVC ASan runtime is a DLL that lives next to cl.exe; copy it
            # beside the test executable so it is found at run time.
            get_filename_component(_msvc_bin_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)
            set(_asan_dll "${_msvc_bin_dir}/clang_rt.asan_dynamic-x86_64.dll")
            if (NOT EXISTS "${_asan_dll}")
                message(FATAL_ERROR
                    "BITBUF_SANITIZE is ON but the ASan runtime was not found at "
                    "'${_asan_dll}'. Install the MSVC 'C++ AddressSanitizer' component, "
                    "or reconfigure with -DBITBUF_SANITIZE=OFF.")
            endif ()
            add_custom_command(TARGET bitbuf_cpp_tests POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${_asan_dll}" "$<TARGET_FILE_DIR:bitbuf_cpp_tests>"
                VERBATIM)
        else ()
            target_compile_options(bitbuf_cpp_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
            target_link_options(bitbuf_cpp_tests PRIVATE -fsanitize=address,undefined)
        endif ()
    endif ()

    add_test(NAME bitbuf_cpp_tests COMMAND bitbuf_cpp_tests)
endif()
