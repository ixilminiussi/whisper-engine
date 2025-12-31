#include <stdexcept>
#include <wsp_texture.hpp>

#include <wsp_assets_manager.hpp>
#include <wsp_constants.hpp>
#include <wsp_device.hpp>
#include <wsp_devkit.hpp>
#include <wsp_image.hpp>
#include <wsp_sampler.hpp>

#include <cgltf.h>

#include <imgui_impl_vulkan.h>

#include <spdlog/spdlog.h>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>

using namespace wsp;

Texture::CreateInfo Texture::GetCreateInfoFromGlTF(cgltf_texture const *texture,
                                                   std::filesystem::path const &parentDirectory)
{
    CreateInfo createInfo{};

    AssetsManager *assetsManager = AssetsManager::Get();

    check(texture);

    cgltf_image *image = texture->image;

    if (image)
    {
        check(image->uri && strncmp(image->uri, "data:", 5) != 0); // we do not support embedded images yet
        Image::CreateInfo imageInfo{};
        imageInfo.filepath = (parentDirectory / image->uri).lexically_normal();
        createInfo.imageInfo = imageInfo;
        createInfo.deferredImageCreation = true;
    }

    if (texture->sampler)
    {
        Sampler::CreateInfo samplerInfo{};
        samplerInfo = Sampler::GetCreateInfoFromGlTF(texture->sampler);
        createInfo.pSampler = AssetsManager::Get()->RequestSampler(samplerInfo);
    }
    else
    {
        createInfo.pSampler = AssetsManager::Get()->RequestSampler({});
    }

    if (texture->name)
    {
        createInfo.name = std::string(texture->name);
    }

    return createInfo;
}

Texture::Texture(Device const *device, CreateInfo const &createInfo) : _name{createInfo.name}, _imageView{}
{
    check(device);

    if (createInfo.deferredImageCreation)
    {
        _image = AssetsManager::Get()->RequestImage(createInfo.imageInfo);
    }
    else
    {
        _image = createInfo.pImage;
    }

    check(_image);
    check(createInfo.pSampler);

    vk::Format const format = _image->GetFormat();
    bool const cubemap = _image->IsCubemap();

    vk::ImageViewCreateInfo viewCreateInfo;
    viewCreateInfo.viewType = cubemap ? vk::ImageViewType::eCube : vk::ImageViewType::e2D;
    viewCreateInfo.format = format;
    viewCreateInfo.image = _image->GetImage();
    viewCreateInfo.subresourceRange.aspectMask =
        createInfo.depth ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;
    viewCreateInfo.subresourceRange.baseMipLevel = 0u;
    viewCreateInfo.subresourceRange.levelCount = _image->GetMipLevels();
    viewCreateInfo.subresourceRange.baseArrayLayer = 0u;
    viewCreateInfo.subresourceRange.layerCount = cubemap ? 6u : 1u;

    _sampler = createInfo.pSampler;

    device->CreateImageView(viewCreateInfo, &_imageView, fmt::format("{}<image_view>", _image->GetName()));

    spdlog::debug("Texture: <{}> -> image: <{}>, format: {}, ?cubemap: {}", _name, _image->GetName(),
                  FormatToString(format), cubemap);
}

Texture::~Texture()
{
    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    device->DestroyImageView(&_imageView);

    spdlog::debug("Texture: <{}> freed", _name);
}

vk::ImageView Texture::GetImageView() const
{
    return _imageView;
}

vk::Sampler Texture::GetSampler() const
{
    check(_sampler);

    return _sampler->GetSampler();
}

Image const *Texture::GetImage() const
{
    check(_image);

    return _image;
}
