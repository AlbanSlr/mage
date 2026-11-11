#pragma once

// The single place where Dear ImGui is included by application code.
//
// Dear ImGui is an "immediate mode" user interface library: there are no
// widget objects to create and keep in sync. Every frame, the application
// simply calls functions describing the interface as it should look right now:
//
//     if (ImGui::Begin("Statistics"))
//     {
//         ImGui::Text("Frame time: %.2f ms", frameTimeInMilliseconds);
//     }
//     ImGui::End();
//
// and Dear ImGui turns those calls into a list of triangles that the renderer
// draws. State such as window positions is tracked by Dear ImGui itself,
// keyed by the names passed to it.
//
// These calls are only valid between UserInterface::beginFrame() and
// UserInterface::endFrame().

#include <imgui.h>
