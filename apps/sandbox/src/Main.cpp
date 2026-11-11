#include <mage/core/Log.hpp>
#include <mage/core/Math.hpp>
#include <mage/platform/Input.hpp>
#include <mage/platform/Window.hpp>
#include <mage/renderer/Camera.hpp>
#include <mage/renderer/Renderer.hpp>

#include <shared/CommandLine.hpp>
#include <shared/DemoScene.hpp>
#include <shared/FlyCameraController.hpp>

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <optional>

// The sandbox is the minimal "game" application: the scene is rendered
// directly into the window, with no editor around it. It is the playground
// used to exercise engine features while they are being developed.
// NOLINT: the C++ standard fixes the signature of main(); its arguments cannot be const.
int main(int argumentCount, char** arguments) // NOLINT(misc-const-correctness)
{
    // Setup failures are reported with exceptions (see docs/CODING_STYLE.md),
    // so the entry point is the single place that catches and reports them.
    try
    {
        const std::optional<std::uint64_t> maxFrameCount = parseMaxFrameCount(argumentCount, arguments);

        mage::log::info("MAGE sandbox starting");
        mage::log::info("Controls: hold right mouse button to look, W/A/S/D to move, E/Q up/down, Shift faster, "
                        "Escape to quit");

        // Declaration order matters: the renderer uses the window, so it is
        // declared after it and therefore destroyed before it.
        mage::platform::Window window(mage::platform::WindowDescription{
            .title = "MAGE Sandbox",
        });

        mage::renderer::Renderer renderer(window,
                                          mage::renderer::RendererDescription{
                                              .applicationName = "MAGE Sandbox",
                                          });

        DemoScene scene(renderer);

        mage::renderer::Camera camera;
        camera.position = {0.0F, 1.5F, 5.0F};
        camera.pitch = glm::radians(-15.0F);

        FlyCameraController cameraController;

        // --- Main loop: one iteration per frame --------------------------------------
        using Clock = std::chrono::steady_clock;

        Clock::time_point previousFrameTime = Clock::now();
        std::uint64_t     renderedFrameCount = 0;

        while (!window.isCloseRequested())
        {
            // Time elapsed since the previous frame, used to make movement
            // and animation independent of the frame rate.
            const Clock::time_point currentFrameTime = Clock::now();
            const float deltaTimeInSeconds = std::chrono::duration<float>(currentFrameTime - previousFrameTime).count();
            previousFrameTime = currentFrameTime;

            window.pollEvents();
            if (window.isKeyDown(mage::platform::Key::Escape))
            {
                break;
            }

            cameraController.update(window, camera, deltaTimeInSeconds);
            scene.update(deltaTimeInSeconds);

            renderer.drawFrame(mage::renderer::FrameDescription{
                .camera = camera,
                .directionToLight = scene.directionToLight,
                .drawCommands = scene.drawCommands(),
            });

            ++renderedFrameCount;
            if (maxFrameCount.has_value() && renderedFrameCount >= *maxFrameCount)
            {
                break;
            }
        }

        mage::log::info("MAGE sandbox shutting down after {} frames", renderedFrameCount);
        return EXIT_SUCCESS;
    }
    catch (const std::exception& exception)
    {
        mage::log::critical("Unhandled exception: {}", exception.what());
        return EXIT_FAILURE;
    }
}
