#include <mage/core/Assert.hpp>
#include <mage/core/Log.hpp>

namespace mage::detail
{
    void reportAssertionFailure(std::string_view            expression,
                                const std::string&          message,
                                const std::source_location& location)
    {
        log::critical("Assertion failed: {}\n    message:  {}\n    location: {}:{}\n    function: {}",
                      expression,
                      message,
                      location.file_name(),
                      location.line(),
                      location.function_name());
        log::flush();
    }
} // namespace mage::detail
