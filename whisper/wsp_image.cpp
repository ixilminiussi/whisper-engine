#include <optional>
#include <spdlog/spdlog.h>
#include <wsp_image.hpp>

#include <wsp_device.hpp>
#include <wsp_devkit.hpp>

#include <cgltf.h>
#include <stb_image.h>

using namespace wsp;

Image *Image::BuildGlTF(Device const *device, cgltf_image const *image, std::filesystem::path const &parentDirectory)
{
    check(device);
    check(image);

    ZoneScopedN("load image");

    if (image->uri && strncmp(image->uri, "data:", 5) != 0) // we're referencing a path
    {
        std::filesystem::path const filepath = (parentDirectory / image->uri).lexically_normal();

        std::string const name = image->name ? std::string(image->name) : "";

        return new Image{device, filepath, name};
    }
    else if (image->uri && strncmp(image->uri, "data:", 5) == 0) // we're on base64
    {
    }
    else if (image->buffer_view) // embedded in buffer
    {
    }
    else // invalid or extension-defined
    {
    }

    return nullptr;
}

vk::Image Image::BuildImage(vk::Format format)
{
    ZoneScopedN("build image");

    check(_filepath.has_value());

    int width, height, channels;
    stbi_uc *pixels = stbi_load(_filepath.value().c_str(), &width, &height, &channels, STBI_rgb);

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

    ImageData &data = _images[format];

    _device->CreateImageAndBindMemory(imageInfo, &data.image, &data.deviceMemory, _name + std::string("_texture"));

    vk::BufferCreateInfo bufferInfo{};
    bufferInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
    bufferInfo.size = width * height * channels;

    vk::Buffer buffer;
    vk::DeviceMemory deviceMemory;
    _device->CreateBufferAndBindMemory(bufferInfo, &buffer, &deviceMemory,
                                       vk::MemoryPropertyFlagBits::eHostVisible |
                                           vk::MemoryPropertyFlagBits::eHostCoherent,
                                       _name + std::string("_texture_buffer"));

    void *memory;
    _device->MapMemory(deviceMemory, &memory);
    memcpy(memory, pixels, width * height * channels);
    _device->UnmapMemory(deviceMemory);

    _device->CopyBufferToImage(buffer, &data.image, width, height);
    _device->DestroyBuffer(&buffer);
    _device->FreeDeviceMemory(&deviceMemory);

    spdlog::info("Image: <{}> -> {} width, {} height, {} channels", _name, width, height, channels);

    stbi_image_free(pixels);

    return data.image;
}

Image::Image(Device const *device, std::filesystem::path const &filepath, std::string const &name)
    : _device{device}, _filepath{filepath}, _name{name}
{
    check(device);
}

Image::Image(Device const *device, vk::ImageCreateInfo createInfo, std::string const &name)
    : _device{device}, _name{name}, _filepath{std::nullopt}
{
    check(device);

    ImageData &data = _images[createInfo.format];
    device->CreateImageAndBindMemory(createInfo, &data.image, &data.deviceMemory, name);
}

Image::~Image()
{
    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    for (auto &[format, imageData] : _images)
    {
        device->DestroyImage(&imageData.image);
        device->FreeDeviceMemory(&imageData.deviceMemory);
    }

    spdlog::debug("Image: <{}> freed", _name);
}

bool Image::AskForVariant(vk::Format format)
{
    if (!_filepath.has_value() && !_images.contains(format))
    {
        return false;
    }

    if (!_images.contains(format))
    {
        BuildImage(format);
    }
    return true;
}

vk::Image Image::GetImage(vk::Format format) const
{
    check(_images.contains(format));

    return _images.at(format).image;
}
