#include <wsp_texture.hpp>

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

Texture::Builder::Builder() : _sampler{nullptr}
{
    _createInfo.viewType = vk::ImageViewType::e2D;
    _createInfo.format = vk::Format::eR8G8B8Srgb;
    _createInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    _createInfo.subresourceRange.baseMipLevel = 0;
    _createInfo.subresourceRange.levelCount = 1;
    _createInfo.subresourceRange.baseArrayLayer = 0;
    _createInfo.subresourceRange.layerCount = 1;
}

Texture::Builder &Texture::Builder::Depth()
{
    _createInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;

    return *this;
}

Texture::Builder &Texture::Builder::SetImage(Image const *image)
{
    check(image);

    _createInfo.image = image->GetImage();

    return *this;
}

Texture::Builder &Texture::Builder::SetSampler(Sampler const *sampler)
{
    _sampler = sampler;

    return *this;
}

Texture::Builder &Texture::Builder::Format(vk::Format format)
{
    _createInfo.format = format;

    return *this;
}

Texture::Builder &Texture::Builder::Name(std::string const &name)
{
    _name = name;

    return *this;
}

Texture::Builder &Texture::Builder::GlTF(cgltf_texture const *texture, cgltf_image *const pImage,
                                         std::vector<Image *> const &images, cgltf_sampler *const pSampler,
                                         std::vector<Sampler *> const &samplers)
{
    check(texture);
    check(pImage);

    check(samplers.size() > 0 && "Texture: you must provide at least one candidate sampler, regardless of GlTF specs");

    cgltf_image *image = texture->image;
    size_t const imageID = image - pImage;

    check(images.size() > imageID);

    _createInfo.image = images.at(imageID)->GetImage();

    if (texture->sampler)
    {
        check(pSampler);
        size_t const samplerID = texture->sampler - pSampler;

        check(samplers.size() > samplerID);
        _sampler = samplers.at(samplerID);
    }
    else
    {
        _sampler = samplers.at(0);
    }

    if (texture->name)
    {
        _name = std::string(texture->name);
    }

    return *this;
}

Texture *Texture::Builder::Build(Device const *device)
{
    check(_sampler);
    check(_createInfo.image);

    return new Texture{device, _createInfo, _sampler, _name};
}

Texture::Texture(Device const *device, vk::ImageViewCreateInfo const &createInfo, Sampler const *sampler,
                 std::string const &name)
    : _name{name}, _sampler{sampler}
{
    check(device);

    device->CreateImageView(createInfo, &_imageView, "_image_view");
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

size_t Texture::GetID() const
{
    return _ID;
}

void Texture::SetID(size_t ID)
{
    _ID = ID;
}
