#include <mage/core/Assert.hpp>
#include <mage/core/Log.hpp>

#include <cstdlib>
#include <exception>

// The sandbox is the playground application used to exercise engine features
// while they are being developed. For now it only proves that the toolchain,
// the core module and the logging backend work end to end.
int main()
{
    // Setup failures are reported with exceptions (see docs/CODING_STYLE.md),
    // so the entry point is the single place that catches and reports them.
    try
    {
        mage::log::info("MAGE sandbox starting");

        const int engineModuleCount = 1;
        MAGE_ASSERT(engineModuleCount > 0, "The sandbox needs at least the core module");

        mage::log::info("MAGE sandbox shutting down");
        return EXIT_SUCCESS;
    }
    catch (const std::exception& exception)
    {
        mage::log::critical("Unhandled exception: {}", exception.what());
        return EXIT_FAILURE;
    }
}
