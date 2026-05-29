#include "test_common.h"

TEST_SUITE_BEGIN("clear");

BITBUF_TEST(clear_zeroes_and_preserves_length) {
    BitBuf b{};
    b.assign_ones(95);
    b.clear();
    CHECK(b.len() == 95u);
    CHECK(b.get_bit(0) == 0);
    CHECK(b.get_bit(94) == 0);
}

TEST_SUITE_END();
