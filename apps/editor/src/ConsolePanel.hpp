#pragma once

#include <mage/core/Log.hpp>

#include <cstddef>
#include <deque>
#include <mutex>
#include <string>
#include <string_view>

// The editor panel showing the engine log.
//
// It registers itself as a log sink (see mage::log::addSink) for its whole
// lifetime, so it should be created as early as possible: messages logged
// before it exists are not shown.
class ConsolePanel
{
public:
    ConsolePanel();
    ~ConsolePanel();

    ConsolePanel(const ConsolePanel&) = delete;
    ConsolePanel& operator=(const ConsolePanel&) = delete;
    ConsolePanel(ConsolePanel&&) = delete;
    ConsolePanel& operator=(ConsolePanel&&) = delete;

    // Draws the panel. `isOpen` is set to false when the user closes it.
    void draw(bool& isOpen);

private:
    struct Entry
    {
        mage::log::Level level;
        std::string      message;
    };

    // Oldest messages are dropped beyond this count.
    static constexpr std::size_t MAX_ENTRY_COUNT = 2000;

    void addEntry(mage::log::Level level, std::string_view message);

    mage::log::SinkHandle m_sinkHandle{};

    // Messages can be logged from any thread, while the panel is drawn from
    // the main thread: the entries are protected by a mutex.
    std::mutex        m_entriesMutex;
    std::deque<Entry> m_entries;

    mage::log::Level m_minimumLevel = mage::log::Level::Trace;
    bool             m_autoScroll = true;
    bool             m_scrollToBottom = false;
};
