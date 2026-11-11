#include "ConsolePanel.hpp"

#include <mage/core/Log.hpp>
#include <mage/ui/ImGui.hpp>

#include <array>
#include <cstddef>
#include <mutex>
#include <string>
#include <string_view>

namespace
{
    // Indexed by mage::log::Level.
    constexpr std::array<const char*, 6> LEVEL_NAMES = {"Trace", "Debug", "Info", "Warning", "Error", "Critical"};

    ImVec4 colorOfLevel(mage::log::Level level)
    {
        switch (level)
        {
            case mage::log::Level::Trace:
            case mage::log::Level::Debug:
                return {0.55F, 0.55F, 0.55F, 1.0F}; // grey
            case mage::log::Level::Info:
                return {0.85F, 0.85F, 0.85F, 1.0F}; // light grey
            case mage::log::Level::Warning:
                return {1.0F, 0.80F, 0.30F, 1.0F}; // yellow
            case mage::log::Level::Error:
            case mage::log::Level::Critical:
                return {1.0F, 0.40F, 0.35F, 1.0F}; // red
        }
        return {1.0F, 1.0F, 1.0F, 1.0F};
    }
} // namespace

ConsolePanel::ConsolePanel()
    : m_sinkHandle(
          mage::log::addSink([this](mage::log::Level level, std::string_view message) { addEntry(level, message); }))
{
}

ConsolePanel::~ConsolePanel()
{
    mage::log::removeSink(m_sinkHandle);
}

void ConsolePanel::addEntry(mage::log::Level level, std::string_view message)
{
    const std::scoped_lock lock(m_entriesMutex);

    m_entries.push_back(Entry{.level = level, .message = std::string(message)});
    if (m_entries.size() > MAX_ENTRY_COUNT)
    {
        m_entries.pop_front();
    }

    m_scrollToBottom = m_autoScroll;
}

void ConsolePanel::draw(bool& isOpen)
{
    // Begin() returns false when the window is collapsed or hidden behind a
    // tab: its content can then be skipped, but End() must always be called.
    if (ImGui::Begin("Console", &isOpen))
    {
        const std::scoped_lock lock(m_entriesMutex);

        // --- Toolbar -----------------------------------------------------------------
        if (ImGui::Button("Clear"))
        {
            m_entries.clear();
        }

        ImGui::SameLine();
        ImGui::Checkbox("Auto-scroll", &m_autoScroll);

        ImGui::SameLine();
        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8.0F);
        int minimumLevelIndex = static_cast<int>(m_minimumLevel);
        if (ImGui::Combo("Minimum level", &minimumLevelIndex, LEVEL_NAMES.data(), static_cast<int>(LEVEL_NAMES.size())))
        {
            m_minimumLevel = static_cast<mage::log::Level>(minimumLevelIndex);
        }

        ImGui::Separator();

        // --- Messages ----------------------------------------------------------------
        // A child window gives the message list its own scrollbar, leaving
        // the toolbar above it always visible.
        if (ImGui::BeginChild(
                "Messages", ImVec2(0.0F, 0.0F), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar))
        {
            for (const Entry& entry : m_entries)
            {
                if (entry.level < m_minimumLevel)
                {
                    continue;
                }

                ImGui::PushStyleColor(ImGuiCol_Text, colorOfLevel(entry.level));
                ImGui::TextUnformatted(entry.message.data(), entry.message.data() + entry.message.size());
                ImGui::PopStyleColor();
            }

            if (m_scrollToBottom)
            {
                ImGui::SetScrollHereY(1.0F);
                m_scrollToBottom = false;
            }
        }
        ImGui::EndChild();
    }
    ImGui::End();
}
