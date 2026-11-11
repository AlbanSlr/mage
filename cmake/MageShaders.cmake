# Shader compilation: Slang source (.slang) -> SPIR-V binary (.spv).
#
# Vulkan does not consume shader source code. It consumes SPIR-V, a binary
# intermediate format, so every shader is compiled ahead of time by `slangc`
# (shipped with the Vulkan SDK) as a normal build step.

find_program(MAGE_SLANGC_EXECUTABLE
    NAMES slangc
    HINTS "$ENV{VULKAN_SDK}/Bin" "$ENV{VULKAN_SDK}/bin"
    REQUIRED
)

# Compiled shaders are placed next to the executables, where the engine looks
# for them at runtime: build/<preset>/bin/<Config>/shaders/<name>.spv
set(MAGE_SHADER_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders")

# mage_add_shaders(<target>
#     SOURCES <file.slang>...
# )
#
# Creates the custom target <target>, which compiles each Slang file into one
# SPIR-V module named after the source file (triangle.slang -> triangle.spv).
# Make an executable depend on <target> so its shaders are always up to date.
#
# Every function marked [shader("...")] in a file becomes an entry point of the
# same SPIR-V module, and keeps its own name (see -fvk-use-entrypoint-name).
function(mage_add_shaders target_name)
    cmake_parse_arguments(PARSE_ARGV 1 SHADERS "" "" "SOURCES")

    set(compiled_shaders "")

    foreach(shader_source IN LISTS SHADERS_SOURCES)
        cmake_path(ABSOLUTE_PATH shader_source OUTPUT_VARIABLE shader_source_path)
        cmake_path(GET shader_source_path STEM shader_name)
        set(compiled_shader "${MAGE_SHADER_OUTPUT_DIRECTORY}/${shader_name}.spv")

        add_custom_command(
            OUTPUT "${compiled_shader}"
            COMMAND "${CMAKE_COMMAND}" -E make_directory "${MAGE_SHADER_OUTPUT_DIRECTORY}"
            COMMAND "${MAGE_SLANGC_EXECUTABLE}" "${shader_source_path}"
                -target spirv
                -profile spirv_1_6            # SPIR-V 1.6 is the version guaranteed by Vulkan 1.3+
                -emit-spirv-directly          # use Slang's own SPIR-V backend, not GLSL as a middle step
                -fvk-use-entrypoint-name      # keep "vertexMain"/"fragmentMain" instead of renaming to "main"
                -matrix-layout-column-major   # same matrix memory layout as GLM
                -warnings-as-errors all
                -o "${compiled_shader}"
            DEPENDS "${shader_source_path}"
            COMMENT "Compiling shader ${shader_name}.slang -> ${shader_name}.spv"
            VERBATIM
        )

        list(APPEND compiled_shaders "${compiled_shader}")
    endforeach()

    add_custom_target(${target_name} ALL
        DEPENDS ${compiled_shaders}
        SOURCES ${SHADERS_SOURCES}
    )
endfunction()
