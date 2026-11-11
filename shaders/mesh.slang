// Textured, lit mesh.
//
// A shader reads three kinds of data, from the least to the most frequently
// changing:
//
//   Per frame   (descriptor set 0)  camera and light, shared by every object
//   Per material(descriptor set 1)  the texture
//   Per object  (push constants)    where the object is in the world
//
// Grouping resources by how often they change is what lets the renderer rebind
// only what differs between two draw calls.

// --- Per frame: descriptor set 0 -------------------------------------------------
// Must match `FrameUniforms` in engine/renderer/src/vulkan/MeshPipeline.hpp.
struct FrameUniforms
{
    // World space -> clip space (projection * view).
    float4x4 viewProjectionMatrix;

    // Normalized direction from a surface toward the light, in world space.
    // Only xyz is used; float4 keeps the layout identical on the C++ side.
    float4 directionToLight;
};

// [[vk::binding(binding, set)]] is the address of a resource: the C++ code
// declares a descriptor of the matching type at the same binding and set.
// A ConstantBuffer is a UNIFORM BUFFER: a small block of read-only data.
[[vk::binding(0, 0)]]
ConstantBuffer<FrameUniforms> frame;

// --- Per material: descriptor set 1 ----------------------------------------------
// A Sampler2D is a COMBINED IMAGE SAMPLER: a texture together with the sampler
// that says how to read it (filtering, wrapping).
[[vk::binding(0, 1)]]
Sampler2D albedoTexture;

// --- Per object: push constants --------------------------------------------------
// Must match `ObjectPushConstants` in engine/renderer/src/vulkan/MeshPipeline.hpp.
struct ObjectConstants
{
    // Local (model) space -> world space.
    float4x4 modelMatrix;

    // Transforms normals to world space. It is the inverse transpose of the
    // model matrix, which keeps normals perpendicular to their surface even
    // when the object is scaled non-uniformly.
    float4x4 normalMatrix;
};

// PUSH CONSTANTS are a few bytes of data stored directly in the command
// buffer: the cheapest way to pass per-draw data, with no buffer to manage.
[[vk::push_constant]]
ConstantBuffer<ObjectConstants> object;

// --- Vertex shader ---------------------------------------------------------------

// One vertex, as stored in the vertex buffer. The locations must match the
// vertex attribute descriptions given to the pipeline (MeshPipeline.cpp).
struct VertexInput
{
    [[vk::location(0)]] float3 position           : POSITION;
    [[vk::location(1)]] float3 normal             : NORMAL;
    [[vk::location(2)]] float2 textureCoordinates : TEXCOORD0;
};

struct VertexOutput
{
    float4 clipPosition       : SV_Position;
    float3 worldNormal        : NORMAL;
    float2 textureCoordinates : TEXCOORD0;
};

// Matrices follow the same convention as the C++ code (see mage/core/Math.hpp):
// mul(matrix, vector) transforms a column vector, so transformations are
// applied from right to left.
[shader("vertex")]
VertexOutput vertexMain(VertexInput input)
{
    const float4 worldPosition = mul(object.modelMatrix, float4(input.position, 1.0));

    VertexOutput output;
    output.clipPosition = mul(frame.viewProjectionMatrix, worldPosition);

    // w = 0: a normal is a direction, so the translation must not apply to it.
    output.worldNormal        = mul(object.normalMatrix, float4(input.normal, 0.0)).xyz;
    output.textureCoordinates = input.textureCoordinates;
    return output;
}

// --- Fragment shader -------------------------------------------------------------

// Fraction of light that reaches every surface regardless of its orientation,
// so faces turned away from the light are dark instead of pure black.
static const float AMBIENT_LIGHT = 0.15;

[shader("fragment")]
float4 fragmentMain(VertexOutput input) : SV_Target
{
    const float3 albedo = albedoTexture.Sample(input.textureCoordinates).rgb;

    // Interpolating normals across a triangle shortens them: renormalize.
    const float3 normal = normalize(input.worldNormal);

    // Lambert diffuse lighting: brightness is the cosine of the angle between
    // the surface normal and the direction to the light.
    const float diffuseLight = max(dot(normal, frame.directionToLight.xyz), 0.0);

    const float3 color = albedo * (AMBIENT_LIGHT + (1.0 - AMBIENT_LIGHT) * diffuseLight);
    return float4(color, 1.0);
}
