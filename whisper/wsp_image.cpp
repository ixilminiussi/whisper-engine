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

Image::Image(Device const *device, CreateInfo const &createInfo, bool cubemap) : _name{createInfo.filepath.filename()}
{
    check(device);

    ZoneScopedN("build image");

    int width, height, channels, size;

    auto build = [&](void *pixels) {
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

        if (cubemap)
        {
            BuildCubemap(device, pixels, width, height, size, channels, _format);
        }
        else
        {
            BuildImage(device, pixels, width, height, size, channels, _format);
        }
    };

    if (createInfo.filepath.extension().compare(".png") == 0)
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

        build(pixels);

        free(pixels);
    }
    else
    {
        throw std::invalid_argument(
            fmt::format("Image: unsupported file format '{}'", createInfo.filepath.extension().string()));
    }
    spdlog::info("Image: <{}> -> {} width, {} height, {} channels, {} size", GetName(), width, height, channels, size);
}

Image::Image(Device const *device, vk::ImageCreateInfo const &createInfo, std::string const &name) : _name{""}
{
    check(device);

    _format = createInfo.format;

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

void Image::BuildImage(Device const *device, void *pixels, uint32_t width, uint32_t height, size_t size,
                       uint32_t channels, vk::Format format)
{
    check(device);

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
    imageInfo.extent = vk::Extent3D{(uint32_t)width, (uint32_t)height, 1u};
    imageInfo.format = format;
    imageInfo.usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
    imageInfo.tiling = vk::ImageTiling::eOptimal;
    imageInfo.mipLevels = 1u;
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
}

void Image::BuildCubemap(Device const *device, void *pixels, uint32_t width, uint32_t height, size_t size,
                         uint32_t channels, vk::Format format)
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
    imageInfo.usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
    imageInfo.tiling = vk::ImageTiling::eOptimal;
    imageInfo.mipLevels = 1u;
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
