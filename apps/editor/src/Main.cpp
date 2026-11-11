#include "ConsolePanel.hpp"
#include "Editor.hpp"

#include <mage/core/Log.hpp>
#include <mage/platform/Window.hpp>
#include <mage/renderer/Renderer.hpp>
#include <mage/ui/UserInterface.hpp>

#include <shared/CommandLine.hpp>

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <optional>

// The MAGE editor: the scene is rendered into an offscreen image shown in a
// viewport panel, surrounded by dockable tool panels.
// NOLINT: the C++ standard fixes the signature of main(); its arguments cannot be const.
int main(int argumentCount, char** arguments) // NOLINT(misc-const-correctness)
{
    // Setup failures are reported with exceptions (see docs/CODING_STYLE.md),
    // so the entry point is the single place that catches and reports them.
    try
    {
        // Created first so that it records the start-up messages as well.
        ConsolePanel consolePanel;

        const std::optional<std::uint64_t> maxFrameCount = parseMaxFrameCount(argumentCount, arguments);

        mage::log::info("MAGE editor starting");

        // Declaration order matters, because objects are destroyed in the
        // opposite order:
        //   window  ->  user interface  ->  renderer  ->  editor
        // The user interface needs the window; the renderer draws the user
        // interface and must be destroyed before it; the editor uses all three.
        mage::platform::Window window(mage::platform::WindowDescription{
            .title = "MAGE Editor",
            .width = 1600,
            .height = 900,
        });

        mage::ui::UserInterface userInterface(window);

        mage::renderer::Renderer renderer(window,
                                          mage::renderer::RendererDescription{
                                              .applicationName = "MAGE Editor",
                                              .enableUserInterface = true,
                                          });

        Editor editor(window, renderer, consolePanel);

        // --- Main loop: one iteration per frame --------------------------------------
        using Clock = std::chrono::steady_clock;

        Clock::time_point previousFrameTime = Clock::now();
        std::uint64_t     renderedFrameCount = 0;

        while (!window.isCloseRequested() && !editor.isExitRequested())
        {
            const Clock::time_point currentFrameTime = Clock::now();
            const float deltaTimeInSeconds = std::chrono::duration<float>(currentFrameTime - previousFrameTime).count();
            previousFrameTime = currentFrameTime;

            // 1. Input: the events also reach Dear ImGui (see ui::UserInterface).
            window.pollEvents();

            // 2. Build the interface. Nothing is drawn yet: the ImGui calls
            //    made by the editor only describe this frame's interface.
            renderer.beginUserInterfaceFrame();
            userInterface.beginFrame();
            editor.update(deltaTimeInSeconds);
            userInterface.endFrame();

            // 3. Render: the scene into its offscreen image, then the
            //    interface (which displays that image) into the window.
            renderer.drawFrame(editor.frameDescription());

            ++renderedFrameCount;
            if (maxFrameCount.has_value() && renderedFrameCount >= *maxFrameCount)
            {
                break;
            }
        }

        mage::log::info("MAGE editor shutting down after {} frames", renderedFrameCount);
        return EXIT_SUCCESS;
    }
    catch (const std::exception& exception)
    {
        mage::log::critical("Unhandled exception: {}", exception.what());
        return EXIT_FAILURE;
    }
}
