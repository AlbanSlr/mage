// First triangle: three hard-coded vertices, one colour per corner.
//
// There is no vertex buffer yet. The vertex shader is simply invoked three
// times (the draw call asks for 3 vertices) and picks its position and colour
// from the constant arrays below using the index of the vertex being processed.

// Positions are given directly in Vulkan's normalized device coordinates (NDC):
//   x: -1 (left)  -> +1 (right)
//   y: -1 (TOP)   -> +1 (BOTTOM)   <- y points down in Vulkan, unlike OpenGL
// The vertices are listed in clockwise order as seen on screen, which the
// pipeline is configured to treat as the front face.
static const float2 VERTEX_POSITIONS[3] = {
    float2( 0.0, -0.5), // top
    float2( 0.5,  0.5), // bottom right
    float2(-0.5,  0.5), // bottom left
};

static const float3 VERTEX_COLORS[3] = {
    float3(1.0, 0.0, 0.0), // red
    float3(0.0, 1.0, 0.0), // green
    float3(0.0, 0.0, 1.0), // blue
};

// Data produced by the vertex shader for each vertex. The rasterizer
// interpolates it across the triangle and hands the result to the fragment
// shader for every covered pixel.
struct VertexOutput
{
    // SV_Position is the one mandatory output: the clip-space position.
    float4 clipPosition : SV_Position;
    float3 color        : COLOR;
};

[shader("vertex")]
VertexOutput vertexMain(uint vertexIndex : SV_VertexID)
{
    VertexOutput output;
    output.clipPosition = float4(VERTEX_POSITIONS[vertexIndex], 0.0, 1.0);
    output.color        = VERTEX_COLORS[vertexIndex];
    return output;
}

// SV_Target is colour attachment 0: the swapchain image being rendered to.
[shader("fragment")]
float4 fragmentMain(VertexOutput input) : SV_Target
{
    return float4(input.color, 1.0);
}
