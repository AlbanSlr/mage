#include <mage/core/Log.hpp>

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <utility>
#include <vector>

namespace
{
    // Records every message received through a log sink for the lifetime of
    // the object, then unregisters itself.
    class LogRecorder
    {
    public:
        LogRecorder()
            : m_sinkHandle(mage::log::addSink([this](mage::log::Level level, std::string_view message)
                                              { m_messages.emplace_back(level, std::string(message)); }))
        {
        }

        ~LogRecorder()
        {
            mage::log::removeSink(m_sinkHandle);
            mage::log::setMinimumLevel(mage::log::Level::Trace);
        }

        LogRecorder(const LogRecorder&) = delete;
        LogRecorder& operator=(const LogRecorder&) = delete;

        [[nodiscard]] const std::vector<std::pair<mage::log::Level, std::string>>& messages() const
        {
            return m_messages;
        }

    private:
        std::vector<std::pair<mage::log::Level, std::string>> m_messages;
        mage::log::SinkHandle                                 m_sinkHandle;
    };
} // namespace

TEST_CASE("Log messages are formatted and delivered to sinks", "[core][log]")
{
    const LogRecorder recorder;

    mage::log::info("Swapchain has {} images of format {}", 3, "B8G8R8A8");

    REQUIRE(recorder.messages().size() == 1);
    CHECK(recorder.messages()[0].first == mage::log::Level::Info);
    CHECK(recorder.messages()[0].second == "Swapchain has 3 images of format B8G8R8A8");
}

TEST_CASE("Each logging function reports its own level", "[core][log]")
{
    const LogRecorder recorder;

    mage::log::trace("trace");
    mage::log::debug("debug");
    mage::log::info("info");
    mage::log::warning("warning");
    mage::log::error("error");
    mage::log::critical("critical");

    REQUIRE(recorder.messages().size() == 6);
    CHECK(recorder.messages()[0].first == mage::log::Level::Trace);
    CHECK(recorder.messages()[1].first == mage::log::Level::Debug);
    CHECK(recorder.messages()[2].first == mage::log::Level::Info);
    CHECK(recorder.messages()[3].first == mage::log::Level::Warning);
    CHECK(recorder.messages()[4].first == mage::log::Level::Error);
    CHECK(recorder.messages()[5].first == mage::log::Level::Critical);
}

TEST_CASE("Messages below the minimum level are discarded", "[core][log]")
{
    const LogRecorder recorder;

    mage::log::setMinimumLevel(mage::log::Level::Warning);

    CHECK_FALSE(mage::log::isEnabled(mage::log::Level::Info));
    CHECK(mage::log::isEnabled(mage::log::Level::Warning));
    CHECK(mage::log::isEnabled(mage::log::Level::Error));

    mage::log::info("discarded");
    mage::log::warning("kept");

    REQUIRE(recorder.messages().size() == 1);
    CHECK(recorder.messages()[0].second == "kept");
}

TEST_CASE("A removed sink no longer receives messages", "[core][log]")
{
    int receivedMessageCount = 0;

    const mage::log::SinkHandle sinkHandle =
        mage::log::addSink([&receivedMessageCount](mage::log::Level, std::string_view) { ++receivedMessageCount; });

    mage::log::info("first");
    mage::log::removeSink(sinkHandle);
    mage::log::info("second");

    CHECK(receivedMessageCount == 1);
}
