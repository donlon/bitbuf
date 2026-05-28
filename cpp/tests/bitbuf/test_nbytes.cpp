#include "test_common.h"

BITBUF_TEST(nbytes_rounds_up) {
    BitBuf b{};
    b.assign_zeros(0);
    ASSERT_EQ(b.nbytes(), 0u);
    b.resize(1);
    ASSERT_EQ(b.nbytes(), 1u);
    b.resize(8);
    ASSERT_EQ(b.nbytes(), 1u);
    b.resize(9);
    ASSERT_EQ(b.nbytes(), 2u);
}
