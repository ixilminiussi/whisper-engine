#include "imgui_impl_vulkan.h"
#include <spdlog/spdlog.h>
#include <vulkan/vulkan_core.h>
#include <wsp_texture.hpp>

#include <wsp_device.hpp>
#include <wsp_devkit.hpp>

#include <vulkan/vulkan.hpp>

using namespace wsp;

Texture::Texture(Device const *device, stbi_uc *pixels, int width, int height, int channels) : _freed{false}
{
    check(device);

    ZoneScopedN("load texture");

    // Create image
    vk::ImageCreateInfo imageInfo{};
    imageInfo.imageType = vk::ImageType::e2D;
    imageInfo.extent = vk::Extent3D{(uint32_t)width, (uint32_t)height, 1u};
    imageInfo.format = vk::Format::eR8G8B8Srgb;
    imageInfo.usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
    imageInfo.tiling = vk::ImageTiling::eOptimal;
    imageInfo.mipLevels = 1u;
    imageInfo.arrayLayers = 1u;

    device->CreateImageAndBindMemory(imageInfo, &_image, &_deviceMemory, "Texture");

    // Create buffer and copy memory
    vk::BufferCreateInfo bufferInfo{};
    bufferInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
    bufferInfo.size = width * height * channels;

    vk::Buffer buffer;
    vk::DeviceMemory deviceMemory;
    device->CreateBufferAndBindMemory(
        bufferInfo, &buffer, &deviceMemory,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, "Texture buffer");

    void *memory;
    device->MapMemory(deviceMemory, &memory);
    memcpy(memory, pixels, width * height * channels);
    device->UnmapMemory(deviceMemory);

    device->CopyBufferToImage(buffer, &_image, width, height);

    vk::ImageViewCreateInfo viewInfo{};
    viewInfo.image = _image;
    viewInfo.viewType = vk::ImageViewType::e2D;
    viewInfo.format = vk::Format::eR8G8B8Srgb;
    viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    device->CreateImageView(viewInfo, &_imageView, "Texture image view");

    device->DestroyBuffer(&buffer);
    device->FreeDeviceMemory(&deviceMemory);

    spdlog::info("Texture: {} width, {} height, {} channels", width, height, channels);
}

Texture::~Texture()
{
    if (!_freed)
    {
        spdlog::critical("Texture: forgot to Free before deletion");
    }
}

void Texture::Free(Device const *device)
{
    if (_freed)
    {
        spdlog::error("Texture: already freed");
        return;
    }

    check(device);

    device->DestroyImageView(&_imageView);
    device->DestroyImage(&_image);
    device->FreeDeviceMemory(&_deviceMemory);

    _freed = true;

    spdlog::info("Texture: freed");
}

vk::ImageView Texture::GetImageView() const
{
    return _imageView;
}

#ifndef NDEBUG
ImTextureID Texture::GetImTextureID(vk::Sampler sampler)
{
    if (!_imguiFlag)
    {
        _imguiID =
            (ImTextureID)ImGui_ImplVulkan_AddTexture(sampler, _imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        _imguiFlag = true;
    }

    return _imguiID;
}
#endif
