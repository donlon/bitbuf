#include "test_common.h"

TEST_SUITE_BEGIN("ensure_buffer");

TEST_CASE_FIXTURE(SingleWordFixture, "ensure_buffer_s1") {
    b.ensure_buffer(-64, 64);
    CHECK(b.offset == 0);
    CHECK(b.length == 128);
    CHECK(!b.using_heap_buffer());
    CHECK(b.inline_buffer[0] == initialPlaceholder); // old
    CHECK(b.inline_buffer[1] == initialWord0);
    CHECK(b.inline_buffer[2] == initialPlaceholder); // old
}

TEST_CASE_FIXTURE(SingleWordFixture, "ensure_buffer_s1_2") {
    b.ensure_buffer(-16, 16);
    CHECK(b.offset == 48);
    CHECK(b.length == 64 + 16);
    CHECK(!b.using_heap_buffer());
    CHECK(b.inline_buffer[0] == initialPlaceholder); // old
    CHECK(b.inline_buffer[1] == initialWord0);
    CHECK(b.inline_buffer[2] == initialPlaceholder); // old
}

TEST_CASE_FIXTURE(SingleWordUnalignedFixture, "ensure_buffer_s1_not_aligned_0") {
    b.ensure_buffer(-64, 64);
    CHECK(b.offset == 12);
    CHECK(b.length == initialLength + 64);
    CHECK(!b.using_heap_buffer());
    CHECK(b.inline_buffer[0] == initialPlaceholder); // old
    CHECK(b.inline_buffer[1] == initialWord0);
    CHECK(b.inline_buffer[2] == initialPlaceholder); // old
}

TEST_CASE_FIXTURE(SingleWordUnalignedFixture, "ensure_buffer_s1_not_aligned_0_1") {
    b.ensure_buffer(-64 - 12, 64 + 12);
    CHECK(b.offset == 0);
    CHECK(b.length == initialLength + 64 + 12);
    CHECK(!b.using_heap_buffer());
    CHECK(b.inline_buffer[0] == initialPlaceholder); // old
    CHECK(b.inline_buffer[1] == initialWord0);
    CHECK(b.inline_buffer[2] == initialPlaceholder); // old
}

TEST_CASE_FIXTURE(SingleWordUnalignedFixture, "ensure_buffer_s1_not_aligned_1") {
    b.ensure_buffer(-8, 16);
    CHECK(b.offset == 64 + 4);
    CHECK(b.length == 64 - 8);
    CHECK(!b.using_heap_buffer());
    CHECK(b.inline_buffer[0] == initialPlaceholder); // old
    CHECK(b.inline_buffer[1] == initialWord0);
    CHECK(b.inline_buffer[2] == initialPlaceholder); // old
}

TEST_CASE_FIXTURE(SingleWordUnalignedFixture, "ensure_buffer_s1_not_aligned_2") {
    b.ensure_buffer(-12, 24);
    CHECK(b.offset == 64);
    CHECK(b.length == 64);
    CHECK(!b.using_heap_buffer());
    CHECK(b.inline_buffer[0] == initialPlaceholder); // old
    CHECK(b.inline_buffer[1] == initialWord0);
    CHECK(b.inline_buffer[2] == initialPlaceholder); // old
}

TEST_CASE_FIXTURE(SingleWordFixture, "ensure_buffer_s1_expand_length") {
    b.ensure_buffer(0, (BitBuf::inlineBufferWords - BitBuf::initialOffsetWords - 1) * 64 - 32);
    CHECK(b.offset == 64);
    CHECK(b.length == (BitBuf::inlineBufferWords - BitBuf::initialOffsetWords) * 64 - 32);
    CHECK(!b.using_heap_buffer());
    CHECK(b.inline_buffer[0] == initialPlaceholder); // old
    CHECK(b.inline_buffer[1] == initialWord0);
    CHECK(b.inline_buffer[2] == initialPlaceholder); // old
}

TEST_CASE_FIXTURE(SingleWordFixture, "ensure_buffer_s1_expand_length_2") {
    b.ensure_buffer(0, (BitBuf::inlineBufferWords - BitBuf::initialOffsetWords - 1) * 64);
    CHECK(b.offset == 64);
    CHECK(b.length == (BitBuf::inlineBufferWords - BitBuf::initialOffsetWords) * 64);
    CHECK(!b.using_heap_buffer());
    CHECK(b.inline_buffer[0] == initialPlaceholder); // old
    CHECK(b.inline_buffer[1] == initialWord0);
    CHECK(b.inline_buffer[2] == initialPlaceholder); // old
}

TEST_CASE_FIXTURE(DoubleWordFixture, "ensure_buffer_s2_move_up") {
    b.ensure_buffer(-128, 0);
    CHECK(b.offset == 0);
    CHECK(b.length == initialLength);
    CHECK(!b.using_heap_buffer());
    CHECK(b.inline_buffer[0] == initialPlaceholder); // old
    CHECK(b.inline_buffer[1] == initialWord0);       // old
    CHECK(b.inline_buffer[2] == initialWord0);
    CHECK(b.inline_buffer[3] == initialWord1);
    // CHECK(b.inline_buffer[4] == initialPlaceholder); // old
}

TEST_CASE_FIXTURE(DoubleWordFixture, "ensure_buffer_s2_move_down") {
    b.ensure_buffer(0, (BitBuf::inlineBufferWords - 2) * 64);
    CHECK(b.offset == 0);
    CHECK(b.length == BitBuf::inlineBufferWords * 64);
    CHECK(!b.using_heap_buffer());
    CHECK(b.inline_buffer[0] == initialWord0);
    CHECK(b.inline_buffer[1] == initialWord1);
    CHECK(b.inline_buffer[2] == initialWord1);       // old
    CHECK(b.inline_buffer[3] == initialPlaceholder); // old
}

TEST_CASE_FIXTURE(DoubleWordUnalignedFixture, "ensure_buffer_s2_move_up_unaligned") {
    b.ensure_buffer(-128, 128);
    CHECK(b.offset == 12);
    CHECK(b.length == initialLength + 128);
    CHECK(!b.using_heap_buffer());
    CHECK(b.inline_buffer[0] == initialPlaceholder); // old
    CHECK(b.inline_buffer[1] == initialWord0);       // old
    CHECK(b.inline_buffer[2] == initialWord0);
    CHECK(b.inline_buffer[3] == initialWord1);
    // CHECK(b.inline_buffer[4] == initialPlaceholder); // old
}

TEST_CASE_FIXTURE(DoubleWordUnalignedFixture, "ensure_buffer_s2_move_up_unaligned_1") {
    b.ensure_buffer(-128 - 12, 128);
    CHECK(b.offset == 0);
    CHECK(b.length == initialLength + 128);
    CHECK(!b.using_heap_buffer());
    CHECK(b.inline_buffer[0] == initialPlaceholder); // old
    CHECK(b.inline_buffer[1] == initialWord0);       // old
    CHECK(b.inline_buffer[2] == initialWord0);
    CHECK(b.inline_buffer[3] == initialWord1);
    // CHECK(b.inline_buffer[4] == initialPlaceholder); // old
}

TEST_CASE_FIXTURE(DoubleWordUnalignedFixture, "ensure_buffer_s2_move_up_unaligned_2") {
    b.ensure_buffer(-128 - 13, 0);
    CHECK(b.offset == 63);
    CHECK(b.length == initialLength);
    CHECK(!b.using_heap_buffer());
    CHECK(b.inline_buffer[0] == initialPlaceholder); // old
    CHECK(b.inline_buffer[1] == initialWord0);       // old
    CHECK(b.inline_buffer[2] == initialWord1);       // old
    CHECK(b.inline_buffer[3] == initialWord0);
    // CHECK(b.inline_buffer[4] == initialWord1);
    // CHECK(b.inline_buffer[5] == initialPlaceholder); // old
}

TEST_CASE_FIXTURE(DoubleWordUnalignedFixture, "ensure_buffer_s2_move_down_unaligned") {
    b.ensure_buffer(0, (BitBuf::inlineBufferWords - BitBuf::initialOffsetWords - 2) * 64 + 13);
    CHECK(b.offset == 12);
    CHECK(b.length == initialLength + (BitBuf::inlineBufferWords - BitBuf::initialOffsetWords - 2) * 64 + 13);
    CHECK(!b.using_heap_buffer());
    CHECK(b.inline_buffer[0] == initialWord0);
    CHECK(b.inline_buffer[1] == initialWord1);
    CHECK(b.inline_buffer[2] == initialWord1);       // old
    CHECK(b.inline_buffer[3] == initialPlaceholder); // old
}

TEST_CASE_FIXTURE(SingleWordFixture, "ensure_buffer_s3_simple") {
    b.ensure_buffer(0, 640);
    CHECK(b.offset == BitBuf::initialHeapOffset);
    CHECK(b.length == initialLength + 640);
    CHECK(b.using_heap_buffer());
    CHECK(b.heap_buffer[BitBuf::initialHeapOffsetWords] == initialWord0);
}

TEST_CASE_FIXTURE(DoubleWordUnalignedFixture, "ensure_buffer_s3_move_up") {
    b.ensure_buffer(-640, 640);
    CHECK(b.offset == 12 + BitBuf::initialHeapOffset);
    CHECK(b.length == initialLength + 640);
    CHECK(b.using_heap_buffer());
    CHECK(b.heap_buffer[BitBuf::initialHeapOffsetWords + 10] == initialWord0);
    CHECK(b.heap_buffer[BitBuf::initialHeapOffsetWords + 11] == initialWord1);
}

TEST_CASE_FIXTURE(DoubleWordUnalignedFixture, "ensure_buffer_s3_move_up_2") {
    b.ensure_buffer(-640 - 12, 640 + 12);
    CHECK(b.offset == BitBuf::initialHeapOffset);
    CHECK(b.length == initialLength + 640 + 12);
    CHECK(b.using_heap_buffer());
    CHECK(b.heap_buffer[BitBuf::initialHeapOffsetWords + 10] == initialWord0);
    CHECK(b.heap_buffer[BitBuf::initialHeapOffsetWords + 11] == initialWord1);
}

TEST_CASE_FIXTURE(DoubleWordUnalignedFixture, "ensure_buffer_s3_move_up_3") {
    b.ensure_buffer(-640 - 13, 640 + 13);
    CHECK(b.offset == BitBuf::initialHeapOffset + 63);
    CHECK(b.length == initialLength + 640 + 13);
    CHECK(b.using_heap_buffer());
    CHECK(b.heap_buffer[BitBuf::initialHeapOffsetWords + 11] == initialWord0);
    CHECK(b.heap_buffer[BitBuf::initialHeapOffsetWords + 12] == initialWord1);
}

TEST_SUITE_END();
