#include <mage/core/Assert.hpp>

#include <catch2/catch_test_macros.hpp>

// A failing assertion aborts the process, so only the passing path can be
// exercised from inside a test.

TEST_CASE("A passing assertion has no effect", "[core][assert]")
{
    const int frameIndex = 1;

    MAGE_ASSERT(frameIndex < 2, "Frame index {} is out of range", frameIndex);

    SUCCEED();
}

TEST_CASE("An assertion evaluates its condition at most once", "[core][assert]")
{
    int evaluationCount = 0;

    MAGE_ASSERT(++evaluationCount > 0, "Condition must hold");

#if defined(MAGE_ENABLE_ASSERTS)
    CHECK(evaluationCount == 1);
#else
    // Release builds compile the assertion out without evaluating it.
    CHECK(evaluationCount == 0);
#endif
}
