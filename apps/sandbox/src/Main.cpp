#include "FlyCameraController.hpp"

#include <mage/core/Log.hpp>
#include <mage/core/Math.hpp>
#include <mage/platform/Input.hpp>
#include <mage/platform/Window.hpp>
#include <mage/renderer/Camera.hpp>
#include <mage/renderer/Mesh.hpp>
#include <mage/renderer/Primitives.hpp>
#include <mage/renderer/Renderer.hpp>
#include <mage/renderer/Texture.hpp>

#include <array>
#include <charconv>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <optional>
#include <string_view>

namespace
{
    // Parses the optional `--max-frames=<count>` argument, which makes the
    // sandbox exit by itself after rendering that many frames. It is used to
    // run the sandbox unattended (automated smoke tests).
    std::optional<std::uint64_t> parseMaxFrameCount(int argumentCount, const char* const* arguments)
    {
        constexpr std::string_view OPTION_PREFIX = "--max-frames=";

        for (int argumentIndex = 1; argumentIndex < argumentCount; ++argumentIndex)
        {
            const std::string_view argument = arguments[argumentIndex];
            if (!argument.starts_with(OPTION_PREFIX))
            {
                continue;
            }

            const std::string_view value = argument.substr(OPTION_PREFIX.size());
            std::uint64_t          maxFrameCount = 0;
            const auto parseResult = std::from_chars(value.data(), value.data() + value.size(), maxFrameCount);
            if (parseResult.ec == std::errc{} && parseResult.ptr == value.data() + value.size())
            {
                return maxFrameCount;
            }

            mage::log::warning("Ignoring invalid argument '{}'", argument);
        }

        return std::nullopt;
    }

    // Builds a model matrix. Read from right to left: the object is first
    // scaled, then rotated around the vertical axis, then moved into place.
    glm::mat4 makeModelMatrix(const glm::vec3& position, float yawInRadians, const glm::vec3& scale)
    {
        constexpr glm::vec3 UP_AXIS{0.0F, 1.0F, 0.0F};

        glm::mat4 modelMatrix{1.0F};
        modelMatrix = glm::translate(modelMatrix, position);
        modelMatrix = glm::rotate(modelMatrix, yawInRadians, UP_AXIS);
        modelMatrix = glm::scale(modelMatrix, scale);
        return modelMatrix;
    }
} // namespace

// The sandbox is the playground application used to exercise engine features
// while they are being developed.
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

        // --- Scene content -----------------------------------------------------------
        // One cube mesh is uploaded once and drawn several times with
        // different transforms and textures.
        const mage::renderer::MeshHandle cubeMesh = renderer.createMesh(mage::renderer::makeCubeMesh());

        const mage::renderer::TextureHandle orangeTexture = renderer.createTexture(
            mage::renderer::makeCheckerboardTexture(8, 32, {235, 120, 40, 255}, {250, 235, 215, 255}));
        const mage::renderer::TextureHandle blueTexture = renderer.createTexture(
            mage::renderer::makeCheckerboardTexture(8, 32, {40, 110, 220, 255}, {220, 235, 250, 255}));
        const mage::renderer::TextureHandle groundTexture = renderer.createTexture(
            mage::renderer::makeCheckerboardTexture(16, 16, {70, 75, 80, 255}, {110, 115, 120, 255}));

        mage::renderer::Camera camera;
        camera.position = {0.0F, 1.5F, 5.0F};
        camera.pitch = glm::radians(-15.0F);

        FlyCameraController cameraController;

        // --- Main loop: one iteration per frame --------------------------------------
        using Clock = std::chrono::steady_clock;

        const Clock::time_point startTime = Clock::now();
        Clock::time_point       previousFrameTime = startTime;
        std::uint64_t           renderedFrameCount = 0;

        while (!window.isCloseRequested())
        {
            // Time elapsed since the previous frame, used to make movement
            // and animation independent of the frame rate.
            const Clock::time_point currentFrameTime = Clock::now();
            const float deltaTimeInSeconds = std::chrono::duration<float>(currentFrameTime - previousFrameTime).count();
            const float elapsedTimeInSeconds = std::chrono::duration<float>(currentFrameTime - startTime).count();
            previousFrameTime = currentFrameTime;

            window.pollEvents();
            if (window.isKeyDown(mage::platform::Key::Escape))
            {
                break;
            }

            cameraController.update(window, camera, deltaTimeInSeconds);

            // The scene: a ground slab, a spinning cube, and a second cube
            // that intersects the first one to show the depth test at work.
            const std::array<mage::renderer::DrawCommand, 3> drawCommands = {
                mage::renderer::DrawCommand{
                    .mesh = cubeMesh,
                    .texture = groundTexture,
                    .modelMatrix = makeModelMatrix({0.0F, -0.55F, 0.0F}, 0.0F, {12.0F, 0.1F, 12.0F}),
                },
                mage::renderer::DrawCommand{
                    .mesh = cubeMesh,
                    .texture = orangeTexture,
                    .modelMatrix = makeModelMatrix({0.0F, 0.0F, 0.0F}, elapsedTimeInSeconds * 0.8F, glm::vec3{1.0F}),
                },
                mage::renderer::DrawCommand{
                    .mesh = cubeMesh,
                    .texture = blueTexture,
                    .modelMatrix = makeModelMatrix({0.9F, 0.25F, -0.6F}, glm::radians(30.0F), glm::vec3{1.5F}),
                },
            };

            renderer.drawFrame(mage::renderer::FrameDescription{
                .camera = camera,
                .directionToLight = {0.4F, 1.0F, 0.6F},
                .drawCommands = drawCommands,
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
