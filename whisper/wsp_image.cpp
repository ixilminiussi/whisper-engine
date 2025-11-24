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

    Image *build = nullptr;

    if (image->uri && strncmp(image->uri, "data:", 5) != 0) // we're referencing a path
    {
        std::filesystem::path const filepath = (parentDirectory / image->uri).lexically_normal();

        int width, height, channels;
        stbi_uc *pixels = stbi_load(filepath.c_str(), &width, &height, &channels, STBI_rgb);

        if (pixels == nullptr)
        {
            throw std::invalid_argument(fmt::format("Image: asset '{}' couldn't be imported", filepath.string()));
        }

        std::string name;

        if (image->name)
        {
            name = std::string(image->name);
        }

        build = new Image{device,
                          (char *)pixels,
                          static_cast<size_t>(width),
                          static_cast<size_t>(height),
                          static_cast<size_t>(channels),
                          name};

        stbi_image_free(pixels);
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

    return build;
}

Image::Image(Device const *device, char *pixels, size_t width, size_t height, size_t channels, std::string const &name)
    : _name{name}
{
    check(device);

    ZoneScopedN("build image");

    vk::ImageCreateInfo imageInfo{};
    imageInfo.imageType = vk::ImageType::e2D;
    imageInfo.extent = vk::Extent3D{(uint32_t)width, (uint32_t)height, 1u};
    imageInfo.format = vk::Format::eR8G8B8Srgb;
    imageInfo.usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
    imageInfo.tiling = vk::ImageTiling::eOptimal;
    imageInfo.mipLevels = 1u;
    imageInfo.arrayLayers = 1u;

    device->CreateImageAndBindMemory(imageInfo, &_image, &_deviceMemory, name + std::string("_texture"));

    vk::BufferCreateInfo bufferInfo{};
    bufferInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
    bufferInfo.size = width * height * channels;

    vk::Buffer buffer;
    vk::DeviceMemory deviceMemory;
    device->CreateBufferAndBindMemory(bufferInfo, &buffer, &deviceMemory,
                                      vk::MemoryPropertyFlagBits::eHostVisible |
                                          vk::MemoryPropertyFlagBits::eHostCoherent,
                                      _name + std::string("_texture_buffer"));

    void *memory;
    device->MapMemory(deviceMemory, &memory);
    memcpy(memory, pixels, width * height * channels);
    device->UnmapMemory(deviceMemory);

    device->CopyBufferToImage(buffer, &_image, width, height);
    device->DestroyBuffer(&buffer);
    device->FreeDeviceMemory(&deviceMemory);

    spdlog::info("Image: <{}> -> {} width, {} height, {} channels", _name, width, height, channels);
}

Image::Image(Device const *device, vk::ImageCreateInfo createInfo, std::string const &name) : _name{name}
{
    device->CreateImageAndBindMemory(createInfo, &_image, &_deviceMemory, name);
}

Image::~Image()
{
    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    device->DestroyImage(&_image);
    device->FreeDeviceMemory(&_deviceMemory);

    spdlog::debug("Image: <{}> freed", _name);
}

vk::Image Image::GetImage() const
{
    return _image;
}
