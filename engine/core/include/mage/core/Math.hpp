#pragma once

// The single place where GLM, the math library, is included.
//
// MAGE math conventions (the same as GLM's defaults, and as glTF):
//
//   - Right-handed coordinate system: +X right, +Y up, -Z forward.
//   - Angles are in radians.
//   - Matrices are column-major and transform column vectors, so
//     transformations compose from right to left:
//         clipPosition = projection * view * model * localPosition
//   - Projection matrices produce depth in the 0..1 range, as Vulkan expects
//     (GLM_FORCE_DEPTH_ZERO_TO_ONE, defined for every module by mage::core).

#include <glm/ext/matrix_clip_space.hpp> // glm::perspective
#include <glm/ext/matrix_transform.hpp>  // glm::translate, glm::rotate, glm::scale, glm::lookAt
#include <glm/ext/scalar_constants.hpp>  // glm::pi
#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp> // glm::radians
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
