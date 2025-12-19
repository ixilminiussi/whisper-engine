#include <wsp_image.hpp>

#include <wsp_device.hpp>
#include <wsp_devkit.hpp>

#include <spdlog/spdlog.h>

#include <cgltf.h>
#include <stb_image.h>

using namespace wsp;

Image::Image(Device const *device, CreateInfo const &createInfo) : _device{device}, _filepath{createInfo.filepath}
{
    check(device);

    BuildImage(createInfo.format);
}

Image::Image(Device const *device, vk::ImageCreateInfo const &createInfo, std::string const &name)
    : _device{device}, _filepath{std::nullopt}
{
    check(device);

    device->CreateImageAndBindMemory(createInfo, &_image, &_deviceMemory, name);
}

std::string Image::GetName() const
{
    if (_filepath.has_value())
    {
        return _filepath.value().filename().string();
    }

    return std::string("");
}

vk::Image Image::BuildImage(vk::Format format, bool cubemap)
{
    if (!ensure(_filepath.has_value()))
    {
        return {};
    }

    ZoneScopedN("build image");

    int width, height, channels;
    stbi_uc *pixels = stbi_load(_filepath.value().c_str(), &width, &height, &channels, STBI_rgb);

    // for now we only support rgb, no alpha
    channels = 3;

    if (pixels == nullptr)
    {
        throw std::invalid_argument(fmt::format("Image: asset '{}' couldn't be imported", _filepath.value().string()));
    }

    vk::ImageCreateInfo imageInfo{};
    imageInfo.imageType = vk::ImageType::e2D;
    imageInfo.extent = vk::Extent3D{(uint32_t)width, (uint32_t)height, 1u};
    imageInfo.format = format;
    imageInfo.usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
    imageInfo.tiling = vk::ImageTiling::eOptimal;
    imageInfo.mipLevels = 1u;
    imageInfo.arrayLayers = 1u;
    if (cubemap)
    {
        imageInfo.extent = vk::Extent3D{(uint32_t)width / 4u, (uint32_t)height / 3u, 1u};
        imageInfo.arrayLayers = 6u;
        imageInfo.flags |= vk::ImageCreateFlagBits::eCubeCompatible;
    }

    _device->CreateImageAndBindMemory(imageInfo, &_image, &_deviceMemory, fmt::format("{}<texture>", GetName()));

    vk::BufferCreateInfo bufferInfo{};
    bufferInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
    bufferInfo.size = width * height * channels;

    vk::Buffer buffer;
    vk::DeviceMemory deviceMemory;
    _device->CreateBufferAndBindMemory(bufferInfo, &buffer, &deviceMemory,
                                       vk::MemoryPropertyFlagBits::eHostVisible |
                                           vk::MemoryPropertyFlagBits::eHostCoherent,
                                       fmt::format("{}<texture_buffer>", GetName()));

    void *memory;
    _device->MapMemory(deviceMemory, &memory);

    if (cubemap)
    {
        int const faceWidth = width / 4u;
        int const faceHeight = height / 4u;

        stbi_uc *cubePixels = (stbi_uc *)malloc(sizeof(stbi_uc) * faceWidth * faceHeight * 6 * channels);

        auto copyFace = [&](int left, int top, int faceID) {
            int const faceGap = faceID * faceWidth * faceHeight * channels;
            for (int y = 0; y < faceHeight; y++)
            {
                int const leftGap = left * faceWidth * channels;
                int const topGap = (y + top * faceHeight) * width * channels;
                memcpy(memory, cubePixels + faceGap + topGap + leftGap, faceWidth * channels);
            }
        };

        copyFace(1, 0, 0);
        copyFace(0, 1, 1);
        copyFace(1, 1, 2);
        copyFace(2, 1, 3);
        copyFace(3, 1, 4);
        copyFace(1, 2, 5);

        _device->UnmapMemory(deviceMemory);

        _device->CopyBufferToImage(buffer, &_image, width, height);

        free(cubePixels);
    }
    else
    {
        memcpy(memory, pixels, width * height * channels);
        _device->UnmapMemory(deviceMemory);

        _device->CopyBufferToImage(buffer, &_image, width, height);
    }
    _device->DestroyBuffer(&buffer);
    _device->FreeDeviceMemory(&deviceMemory);

    spdlog::info("Image: <{}> -> {} width, {} height, {} channels", GetName(), width, height, channels);

    stbi_image_free(pixels);

    return _image;
}

Image::~Image()
{
    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    device->DestroyImage(&_image);
    device->FreeDeviceMemory(&_deviceMemory);

    spdlog::debug("Image: <{}> freed", GetName());
}

vk::Image Image::GetImage() const
{
    return _image;
}
