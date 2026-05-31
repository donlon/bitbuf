#include "test_common.h"

TEST_SUITE_BEGIN("nbytes");

BITBUF_TEST(nbytes_rounds_up) {
    BitBuf b{};
    b.assign_zeros(0);
    CHECK(b.nbytes() == 0u);
    b.resize(1);
    CHECK(b.nbytes() == 1u);
    b.resize(8);
    CHECK(b.nbytes() == 1u);
    b.resize(9);
    CHECK(b.nbytes() == 2u);
}

TEST_SUITE_END();
