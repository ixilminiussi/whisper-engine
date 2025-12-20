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
        imageInfo.format = createInfo.format;
        createInfo.imageInfo = imageInfo;
    }

    if (texture->sampler)
    {
        Sampler::CreateInfo samplerInfo{};
        samplerInfo = Sampler::GetCreateInfoFromGlTF(texture->sampler);
        createInfo.samplerInfo = samplerInfo;
    }
    else
    {
        createInfo.samplerInfo = Sampler::CreateInfo{};
    }

    if (texture->name)
    {
        createInfo.name = std::string(texture->name);
    }

    return createInfo;
}

Texture::Texture(Device const *device, CreateInfo const &createInfo)
    : _name{createInfo.name}, _ID{INVALID_ID}, _imageView{}
{
    check(device);

    Image *image = createInfo.pImage ? createInfo.pImage : AssetsManager::Get()->RequestImage(createInfo.imageInfo);

    vk::ImageViewCreateInfo viewCreateInfo;
    viewCreateInfo.viewType = createInfo.cubemap ? vk::ImageViewType::eCube : vk::ImageViewType::e2D;
    viewCreateInfo.format = createInfo.format;
    viewCreateInfo.image = image->GetImage();
    viewCreateInfo.subresourceRange.aspectMask =
        createInfo.depth ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;
    viewCreateInfo.subresourceRange.baseMipLevel = 0u;
    viewCreateInfo.subresourceRange.levelCount = 1u;
    viewCreateInfo.subresourceRange.baseArrayLayer = 0u;
    viewCreateInfo.subresourceRange.layerCount = createInfo.cubemap ? 6u : 1u;

    _sampler = createInfo.pSampler ? createInfo.pSampler : AssetsManager::Get()->RequestSampler(createInfo.samplerInfo);

    device->CreateImageView(viewCreateInfo, &_imageView, fmt::format("{}<image_view>", image->GetName()));
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

int Texture::GetID() const
{
    return _ID;
}

void Texture::SetID(int ID)
{
    _ID = ID;
}
