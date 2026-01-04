#include <memory>
#include <wsp_image.hpp>

#include <wsp_device.hpp>
#include <wsp_devkit.hpp>
#include <wsp_static_utils.hpp>

#include <spdlog/spdlog.h>

#include <cgltf.h>
#include <stb_image.h>
#include <stdexcept>
#include <tinyexr.h>

using namespace wsp;

Image::Image(Device const *device, CreateInfo const &createInfo)
    : _name{createInfo.filepath.filename()}, _cubemap{createInfo.cubemap}
{
    check(device);

    ZoneScopedN("build image");

    int width, height, channels, size;

    auto build = [&](void *pixels) {
        _mipLevels =
            (uint32_t)std::min((double)createInfo.mipLevels, 1u + std::floor(std::log2(std::max(width, height))));

        if (createInfo.format == vk::Format::eUndefined)
        {
            _format = SelectFormat(channels, size);
        }
        else
        {
            _format = createInfo.format;
        }

        if (!pixels)
        {
            throw std::invalid_argument(
                fmt::format("Image: asset '{}' couldn't be imported", createInfo.filepath.string()));
        }

        if (_cubemap)
        {
            BuildCubemap(device, pixels, width, height, size, channels, _format, _mipLevels);
        }
        else
        {
            BuildImage(device, pixels, width, height, size, channels, _format, _mipLevels);
        }
    };

    if (createInfo.filepath.extension().compare(".png") == 0 || createInfo.filepath.extension().compare(".jpg") == 0 ||
        createInfo.filepath.extension().compare(".jpeg") == 0)
    {
        stbi_uc *pixels = stbi_load(createInfo.filepath.c_str(), &width, &height, &channels, STBI_rgb);

        size = 1;
        channels = 3;

        build(pixels);

        stbi_image_free(pixels);
    }
    else if (createInfo.filepath.extension().compare(".exr") == 0)
    {
        float *pixels;
        char const *err;

        channels = 4;
        size = 4;

        LoadEXR(&pixels, &width, &height, createInfo.filepath.c_str(), &err);
        spdlog::critical("{}w, {}h, {}c, {}s", width, height, channels, size);

        build(pixels);

        free(pixels);
    }
    else
    {
        throw std::invalid_argument(
            fmt::format("Image: unsupported file format '{}'", createInfo.filepath.extension().string()));
    }
    spdlog::info("Image: <{}> -> {} format, {} width, {} height, {} channels, {} size", GetName(),
                 FormatToString(_format), width, height, channels, size);
}

Image::Image(Device const *device, vk::ImageCreateInfo const &createInfo, std::string const &name)
    : _name{""}, _format{createInfo.format}
{
    check(device);

    _cubemap = createInfo.arrayLayers == 6u && createInfo.flags & vk::ImageCreateFlagBits::eCubeCompatible;
    _format = createInfo.format;
    _mipLevels = createInfo.mipLevels;

    device->CreateImageAndBindMemory(createInfo, &_image, &_deviceMemory, name);
}

Image::~Image()
{
    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    device->DestroyImage(&_image);
    device->FreeDeviceMemory(&_deviceMemory);

    spdlog::debug("Image: <{}> freed", GetName());
}

void Image::GenerateMipmaps(Device const *device, vk::Format format, int32_t width, int32_t height, uint32_t mipLevels,
                            uint32_t layerCount)
{
    int32_t previousWidth = width;
    int32_t previousHeight = height;

    vk::CommandBuffer const commandBuffer = device->BeginSingleTimeCommand();

    // transition first image to TransferSrc
    commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, {}, {},
                                  {},
                                  vk::ImageMemoryBarrier{vk::AccessFlagBits::eNone,
                                                         vk::AccessFlagBits::eTransferRead,
                                                         vk::ImageLayout::eUndefined,
                                                         vk::ImageLayout::eTransferSrcOptimal,
                                                         vk::QueueFamilyIgnored,
                                                         vk::QueueFamilyIgnored,
                                                         _image,
                                                         {vk::ImageAspectFlagBits::eColor, 0, 1, 0, layerCount}});

    for (uint32_t i = 1; i < mipLevels; ++i)
    {
        int32_t const mipWidth = std::max(1, (width >> i));
        int32_t const mipHeight = std::max(1, (height >> i));

        vk::ImageBlit blit{};
        blit.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.layerCount = layerCount;
        blit.srcOffsets[1] = vk::Offset3D{previousWidth, previousHeight, 1};

        blit.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.layerCount = layerCount;
        blit.dstOffsets[1] = vk::Offset3D{mipWidth, mipHeight, 1};

        commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, {},
                                      {}, {},
                                      vk::ImageMemoryBarrier{vk::AccessFlagBits::eNone,
                                                             vk::AccessFlagBits::eTransferWrite,
                                                             vk::ImageLayout::eUndefined,
                                                             vk::ImageLayout::eTransferDstOptimal,
                                                             vk::QueueFamilyIgnored,
                                                             vk::QueueFamilyIgnored,
                                                             _image,
                                                             {vk::ImageAspectFlagBits::eColor, i, 1, 0, layerCount}});

        commandBuffer.blitImage(_image, vk::ImageLayout::eTransferSrcOptimal, _image,
                                vk::ImageLayout::eTransferDstOptimal, 1, &blit, vk::Filter::eLinear);

        commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, {},
                                      {}, {},
                                      vk::ImageMemoryBarrier{vk::AccessFlagBits::eTransferWrite,
                                                             vk::AccessFlagBits::eTransferRead,
                                                             vk::ImageLayout::eTransferDstOptimal,
                                                             vk::ImageLayout::eTransferSrcOptimal,
                                                             vk::QueueFamilyIgnored,
                                                             vk::QueueFamilyIgnored,
                                                             _image,
                                                             {vk::ImageAspectFlagBits::eColor, i, 1, 0, layerCount}});

        commandBuffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eAllGraphics, {}, {}, {},
            vk::ImageMemoryBarrier{vk::AccessFlagBits::eTransferRead,
                                   vk::AccessFlagBits::eShaderRead,
                                   vk::ImageLayout::eTransferSrcOptimal,
                                   vk::ImageLayout::eShaderReadOnlyOptimal,
                                   vk::QueueFamilyIgnored,
                                   vk::QueueFamilyIgnored,
                                   _image,
                                   {vk::ImageAspectFlagBits::eColor, i - 1, 1, 0, layerCount}});

        previousWidth = mipWidth;
        previousHeight = mipHeight;
    }

    commandBuffer.pipelineBarrier(
        vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eAllGraphics, {}, {}, {},
        vk::ImageMemoryBarrier{vk::AccessFlagBits::eTransferRead,
                               vk::AccessFlagBits::eShaderRead,
                               vk::ImageLayout::eTransferSrcOptimal,
                               vk::ImageLayout::eShaderReadOnlyOptimal,
                               vk::QueueFamilyIgnored,
                               vk::QueueFamilyIgnored,
                               _image,
                               {vk::ImageAspectFlagBits::eColor, mipLevels - 1, 1, 0, layerCount}});

    device->EndSingleTimeCommand(commandBuffer);
}

void Image::BuildImage(Device const *device, void *pixels, uint32_t width, uint32_t height, size_t size,
                       uint32_t channels, vk::Format format, uint32_t mipLevels)
{
    check(device);

    uint32_t t_channels;
    size_t t_size;

    DecomposeFormat(format, &t_channels, &t_size);

    if (t_size != size)
    {
        throw std::invalid_argument("Image: incompatible format with given size");
    }

    vk::ImageCreateInfo imageInfo{};
    imageInfo.imageType = vk::ImageType::e2D;
    imageInfo.extent = vk::Extent3D{(uint32_t)width, (uint32_t)height, 1u};
    imageInfo.format = format;
    imageInfo.usage =
        vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
    imageInfo.tiling = vk::ImageTiling::eOptimal;
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = 1u;

    device->CreateImageAndBindMemory(imageInfo, &_image, &_deviceMemory, fmt::format("{}<texture>", GetName()));

    vk::BufferCreateInfo bufferInfo{};
    bufferInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
    bufferInfo.size = width * height * t_channels * size;

    vk::Buffer buffer;
    vk::DeviceMemory deviceMemory;
    device->CreateBufferAndBindMemory(bufferInfo, &buffer, &deviceMemory,
                                      vk::MemoryPropertyFlagBits::eHostVisible |
                                          vk::MemoryPropertyFlagBits::eHostCoherent,
                                      fmt::format("{}<texture_buffer>", GetName()));

    void *memory;
    device->MapMemory(deviceMemory, &memory);

    CopyFaceToFace(0, 0, 0, pixels, width, height, channels, size, memory, width, height, t_channels, size);

    device->UnmapMemory(deviceMemory);

    device->CopyBufferToImage(buffer, &_image, width, height, 1, 1);
    device->DestroyBuffer(&buffer);
    device->FreeDeviceMemory(&deviceMemory);

    GenerateMipmaps(device, format, width, height, mipLevels, 1u);
}

void Image::BuildCubemap(Device const *device, void *pixels, uint32_t width, uint32_t height, size_t size,
                         uint32_t channels, vk::Format format, uint32_t mipLevels)
{
    uint32_t const t_width = width;
    uint32_t const t_height = height / 6u;

    uint32_t t_channels;
    size_t t_size;

    DecomposeFormat(format, &t_channels, &t_size);

    if (t_size != size)
    {
        throw std::invalid_argument("Image: incompatible format with given size");
        ;
    }

    vk::ImageCreateInfo imageInfo{};
    imageInfo.imageType = vk::ImageType::e2D;
    imageInfo.extent = vk::Extent3D{t_width, t_height, 1u};
    imageInfo.format = format;
    imageInfo.usage =
        vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
    imageInfo.tiling = vk::ImageTiling::eOptimal;
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = 6u;
    imageInfo.flags |= vk::ImageCreateFlagBits::eCubeCompatible;

    device->CreateImageAndBindMemory(imageInfo, &_image, &_deviceMemory, fmt::format("{}<texture>", GetName()));

    vk::BufferCreateInfo bufferInfo{};
    bufferInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
    bufferInfo.size = t_width * t_height * channels * size * 6u;

    vk::Buffer buffer;
    vk::DeviceMemory deviceMemory;
    device->CreateBufferAndBindMemory(bufferInfo, &buffer, &deviceMemory,
                                      vk::MemoryPropertyFlagBits::eHostVisible |
                                          vk::MemoryPropertyFlagBits::eHostCoherent,
                                      fmt::format("{}<texture_buffer>", GetName()));

    void *memory;
    device->MapMemory(deviceMemory, &memory);

    CopyFaceToFace(0, 0, 0, pixels, width, height, channels, size, memory, t_width, t_height, t_channels, size);
    CopyFaceToFace(0, 1, 1, pixels, width, height, channels, size, memory, t_width, t_height, t_channels, size);
    CopyFaceToFace(0, 2, 2, pixels, width, height, channels, size, memory, t_width, t_height, t_channels, size);
    CopyFaceToFace(0, 3, 3, pixels, width, height, channels, size, memory, t_width, t_height, t_channels, size);
    CopyFaceToFace(0, 5, 4, pixels, width, height, channels, size, memory, t_width, t_height, t_channels, size);
    CopyFaceToFace(0, 4, 5, pixels, width, height, channels, size, memory, t_width, t_height, t_channels, size);

    device->UnmapMemory(deviceMemory);

    device->CopyBufferToImage(buffer, &_image, t_width, t_height, 1, 6);
    device->DestroyBuffer(&buffer);
    device->FreeDeviceMemory(&deviceMemory);

    GenerateMipmaps(device, format, t_width, t_height, mipLevels, 6u);
}

void Image::CopyFaceToFace(uint32_t left, uint32_t top, uint32_t faceID, void *source, uint32_t s_width,
                           uint32_t s_height, uint32_t s_channels, size_t s_size, void *target, uint32_t t_width,
                           uint32_t t_height, uint32_t t_channels, size_t t_size)
{
    check(t_size == s_size);

    uint32_t const size = t_size;

    if (s_width == t_width && s_height == t_height && t_channels == s_channels)
    {
        memcpy(target, source, s_width * s_height * s_channels * size);
    }
    else if (t_channels == s_channels)
    {
        int const faceOffset = faceID * t_width * t_height;
        uint32_t const channels = t_channels;

        for (int y = 0; y < t_height; y++)
        {
            uint32_t const s_offset = ((top * t_height + y) * s_width + (left * t_width)) * channels * size;
            uint32_t const t_offset = (faceOffset + y * t_height) * channels * size;

            int const strip = t_width * channels * size;
            memcpy((std::byte *)target + t_offset, (std::byte *)source + s_offset, strip);
        }
    }
    else
    {
        int const faceOffset = faceID * t_width * t_height;

        uint32_t const minChannels = std::min(t_channels, s_channels);

        for (int y = 0; y < t_height; y++)
        {
            for (int x = 0; x < t_width; x++)
            {
                uint32_t const s_offset = ((top * t_height + y) * s_width + (left * t_width + x)) * s_channels * size;
                uint32_t const t_offset = (faceOffset + y * t_height + x) * t_channels * size;

                int const pixel = minChannels * size;
                memcpy((std::byte *)target + t_offset, (std::byte *)source + s_offset, pixel);
            }
        }
    }
}

std::string Image::GetName() const
{
    return _name;
}

vk::Image Image::GetImage() const
{
    return _image;
}

vk::Format Image::GetFormat() const
{
    return _format;
}

bool Image::IsCubemap() const
{
    return _cubemap;
}

uint32_t Image::GetMipLevels() const
{
    return _mipLevels;
}
