#include "RenderResources.hpp"

#include "vulkan/VulkanAllocator.hpp"
#include "vulkan/VulkanApi.hpp"
#include "vulkan/VulkanBuffer.hpp"
#include "vulkan/VulkanDevice.hpp"
#include "vulkan/VulkanImage.hpp"
#include "vulkan/VulkanImageLayout.hpp"
#include "vulkan/VulkanImmediateCommands.hpp"

#include <mage/core/Assert.hpp>
#include <mage/core/Log.hpp>
#include <mage/renderer/Mesh.hpp>
#include <mage/renderer/Texture.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

namespace mage::renderer
{
    namespace
    {
        // Creates a GpuOnly buffer containing `data`.
        //
        // GPU-only memory cannot be written by the CPU, so the data takes two
        // steps: it is first written to a temporary CPU-writable STAGING
        // buffer, then copied from there to the final buffer by the GPU.
        VulkanBuffer createGpuBufferWithData(const VulkanAllocator&         allocator,
                                             const VulkanImmediateCommands& immediateCommands,
                                             std::span<const std::byte>     data,
                                             vk::BufferUsageFlags           usage)
        {
            const VulkanBuffer stagingBuffer(allocator,
                                             VulkanBufferDescription{
                                                 .sizeInBytes = data.size(),
                                                 .usage = vk::BufferUsageFlagBits::eTransferSrc,
                                                 .memoryAccess = BufferMemoryAccess::CpuWritable,
                                             });
            stagingBuffer.write(data);

            VulkanBuffer gpuBuffer(allocator,
                                   VulkanBufferDescription{
                                       .sizeInBytes = data.size(),
                                       .usage = usage | vk::BufferUsageFlagBits::eTransferDst,
                                       .memoryAccess = BufferMemoryAccess::GpuOnly,
                                   });

            immediateCommands.submit(
                [&stagingBuffer, &gpuBuffer, &data](const vk::raii::CommandBuffer& commandBuffer)
                {
                    const vk::BufferCopy copyRegion{
                        .srcOffset = 0,
                        .dstOffset = 0,
                        .size = data.size(),
                    };
                    commandBuffer.copyBuffer(stagingBuffer.handle(), gpuBuffer.handle(), copyRegion);
                });

            // The staging buffer is destroyed here; the copy has completed.
            return gpuBuffer;
        }
    } // namespace

    RenderResources::RenderResources(const VulkanDevice&            device,
                                     const VulkanAllocator&         allocator,
                                     const VulkanImmediateCommands& immediateCommands,
                                     vk::DescriptorSetLayout        materialDescriptorSetLayout)
        : m_device(device)
        , m_allocator(allocator)
        , m_immediateCommands(immediateCommands)
        , m_materialDescriptorSetLayout(materialDescriptorSetLayout)
    {
        const vk::raii::Device& logicalDevice = m_device.logicalDevice();

        // --- Descriptor pool ----------------------------------------------------------
        // The pool is sized by descriptor type: how many descriptors of each
        // kind can be allocated in total, and how many sets.
        const std::array<vk::DescriptorPoolSize, 2> poolSizes = {
            vk::DescriptorPoolSize{
                .type = vk::DescriptorType::eUniformBuffer,
                .descriptorCount = MAX_FRAME_UNIFORM_COUNT,
            },
            vk::DescriptorPoolSize{
                .type = vk::DescriptorType::eCombinedImageSampler,
                .descriptorCount = MAX_TEXTURE_COUNT,
            },
        };

        m_descriptorPool = logicalDevice.createDescriptorPool(vk::DescriptorPoolCreateInfo{
            // Allows sets to be freed individually, which vk::raii::DescriptorSet
            // does in its destructor. Without this flag a pool can only be
            // reset as a whole.
            .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
            .maxSets = MAX_FRAME_UNIFORM_COUNT + MAX_TEXTURE_COUNT,
            .poolSizeCount = static_cast<std::uint32_t>(poolSizes.size()),
            .pPoolSizes = poolSizes.data(),
        });

        // --- Default sampler ----------------------------------------------------------
        m_defaultSampler = logicalDevice.createSampler(vk::SamplerCreateInfo{
            // Linear filtering blends the four nearest pixels, which smooths
            // the texture when it is magnified or minified.
            .magFilter = vk::Filter::eLinear,
            .minFilter = vk::Filter::eLinear,

            // Textures have a single mip level for now, so this has no effect yet.
            .mipmapMode = vk::SamplerMipmapMode::eLinear,

            // Repeat: coordinates outside 0..1 wrap around, tiling the texture.
            .addressModeU = vk::SamplerAddressMode::eRepeat,
            .addressModeV = vk::SamplerAddressMode::eRepeat,
            .addressModeW = vk::SamplerAddressMode::eRepeat,
        });
    }

    MeshHandle RenderResources::createMesh(const MeshData& meshData)
    {
        if (meshData.vertices.empty() || meshData.indices.empty())
        {
            throw std::invalid_argument("A mesh needs at least one vertex and one index");
        }

        // std::as_bytes views the vertex and index arrays as raw bytes,
        // without copying them.
        m_meshes.push_back(GpuMesh{
            .vertexBuffer = createGpuBufferWithData(m_allocator,
                                                    m_immediateCommands,
                                                    std::as_bytes(std::span(meshData.vertices)),
                                                    vk::BufferUsageFlagBits::eVertexBuffer),
            .indexBuffer = createGpuBufferWithData(m_allocator,
                                                   m_immediateCommands,
                                                   std::as_bytes(std::span(meshData.indices)),
                                                   vk::BufferUsageFlagBits::eIndexBuffer),
            .indexCount = static_cast<std::uint32_t>(meshData.indices.size()),
        });

        log::debug("Created mesh #{}: {} vertices, {} indices",
                   m_meshes.size() - 1,
                   meshData.vertices.size(),
                   meshData.indices.size());

        return MeshHandle{static_cast<std::uint32_t>(m_meshes.size() - 1)};
    }

    TextureHandle RenderResources::createTexture(const TextureData& textureData)
    {
        constexpr std::size_t BYTES_PER_PIXEL = 4;

        const std::size_t expectedByteCount =
            static_cast<std::size_t>(textureData.width) * textureData.height * BYTES_PER_PIXEL;
        if (expectedByteCount == 0 || textureData.rgbaPixels.size() != expectedByteCount)
        {
            throw std::invalid_argument("Texture pixel data does not match its width and height");
        }
        if (m_textures.size() >= MAX_TEXTURE_COUNT)
        {
            throw std::runtime_error("The maximum number of textures has been reached");
        }

        const vk::raii::Device& logicalDevice = m_device.logicalDevice();
        const vk::Extent2D      extent{.width = textureData.width, .height = textureData.height};

        // --- 1. Pixels go to a staging buffer first, like mesh data ------------------
        const VulkanBuffer stagingBuffer(m_allocator,
                                         VulkanBufferDescription{
                                             .sizeInBytes = textureData.rgbaPixels.size(),
                                             .usage = vk::BufferUsageFlagBits::eTransferSrc,
                                             .memoryAccess = BufferMemoryAccess::CpuWritable,
                                         });
        stagingBuffer.write(std::as_bytes(std::span(textureData.rgbaPixels)));

        // --- 2. Create the image ------------------------------------------------------
        VulkanImage image(m_device,
                          m_allocator,
                          VulkanImageDescription{
                              .extent = extent,

                              // 8 bits per channel, sRGB encoded. The GPU converts to
                              // linear values when a shader samples the texture, so
                              // lighting is computed with physically meaningful numbers.
                              .format = vk::Format::eR8G8B8A8Srgb,

                              // Sampled by shaders, and destination of the copy below.
                              .usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst,
                              .aspect = vk::ImageAspectFlagBits::eColor,
                          });

        // --- 3. Copy the pixels into it -----------------------------------------------
        m_immediateCommands.submit(
            [&stagingBuffer, &image, extent](const vk::raii::CommandBuffer& commandBuffer)
            {
                // Undefined -> TransferDstOptimal: the layout required to be
                // the destination of a copy.
                transitionImageLayout(commandBuffer,
                                      image.handle(),
                                      ImageLayoutTransition{
                                          .oldLayout = vk::ImageLayout::eUndefined,
                                          .newLayout = vk::ImageLayout::eTransferDstOptimal,
                                          .sourceStage = vk::PipelineStageFlagBits2::eNone,
                                          .sourceAccess = vk::AccessFlagBits2::eNone,
                                          .destinationStage = vk::PipelineStageFlagBits2::eCopy,
                                          .destinationAccess = vk::AccessFlagBits2::eTransferWrite,
                                      });

                const vk::BufferImageCopy copyRegion{
                    .bufferOffset = 0,

                    // 0 means the rows in the buffer are tightly packed.
                    .bufferRowLength = 0,
                    .bufferImageHeight = 0,

                    .imageSubresource =
                        vk::ImageSubresourceLayers{
                            .aspectMask = vk::ImageAspectFlagBits::eColor,
                            .mipLevel = 0,
                            .baseArrayLayer = 0,
                            .layerCount = 1,
                        },
                    .imageOffset = vk::Offset3D{.x = 0, .y = 0, .z = 0},
                    .imageExtent = vk::Extent3D{.width = extent.width, .height = extent.height, .depth = 1},
                };

                commandBuffer.copyBufferToImage(
                    stagingBuffer.handle(), image.handle(), vk::ImageLayout::eTransferDstOptimal, copyRegion);

                // TransferDstOptimal -> ShaderReadOnlyOptimal: the layout
                // required to be sampled, which the image keeps from now on.
                transitionImageLayout(commandBuffer,
                                      image.handle(),
                                      ImageLayoutTransition{
                                          .oldLayout = vk::ImageLayout::eTransferDstOptimal,
                                          .newLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
                                          .sourceStage = vk::PipelineStageFlagBits2::eCopy,
                                          .sourceAccess = vk::AccessFlagBits2::eTransferWrite,
                                          .destinationStage = vk::PipelineStageFlagBits2::eFragmentShader,
                                          .destinationAccess = vk::AccessFlagBits2::eShaderSampledRead,
                                      });
            });

        // --- 4. Create the descriptor set that exposes it to shaders -----------------
        std::vector<vk::raii::DescriptorSet> descriptorSets =
            logicalDevice.allocateDescriptorSets(vk::DescriptorSetAllocateInfo{
                .descriptorPool = *m_descriptorPool,
                .descriptorSetCount = 1,
                .pSetLayouts = &m_materialDescriptorSetLayout,
            });

        // A newly allocated set is empty. "Writing" a descriptor makes it
        // point at an actual resource.
        const vk::DescriptorImageInfo imageInfo{
            .sampler = *m_defaultSampler,
            .imageView = image.view(),
            .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
        };

        const vk::WriteDescriptorSet descriptorWrite{
            .dstSet = *descriptorSets.front(),
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .pImageInfo = &imageInfo,
        };

        logicalDevice.updateDescriptorSets(descriptorWrite, nullptr);

        m_textures.push_back(GpuTexture{
            .image = std::move(image),
            .materialDescriptorSet = std::move(descriptorSets.front()),
        });

        log::debug("Created texture #{}: {}x{}", m_textures.size() - 1, textureData.width, textureData.height);

        return TextureHandle{static_cast<std::uint32_t>(m_textures.size() - 1)};
    }

    const GpuMesh& RenderResources::mesh(MeshHandle meshHandle) const
    {
        const auto meshIndex = static_cast<std::uint32_t>(meshHandle);
        MAGE_ASSERT(meshIndex < m_meshes.size(), "Invalid mesh handle {}", meshIndex);
        return m_meshes[meshIndex];
    }

    const GpuTexture& RenderResources::texture(TextureHandle textureHandle) const
    {
        const auto textureIndex = static_cast<std::uint32_t>(textureHandle);
        MAGE_ASSERT(textureIndex < m_textures.size(), "Invalid texture handle {}", textureIndex);
        return m_textures[textureIndex];
    }

    const vk::raii::DescriptorPool& RenderResources::descriptorPool() const
    {
        return m_descriptorPool;
    }
} // namespace mage::renderer
