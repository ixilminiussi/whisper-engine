#include <wsp_texture.hpp>

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

Texture::Builder::Builder() : _sampler{nullptr}
{
    _format = vk::Format::eR8G8B8Srgb;
    _depth = false;
    _image = nullptr;
    _sampler = nullptr;
}

Texture::Builder &Texture::Builder::Depth()
{
    _depth = true;

    return *this;
}

Texture::Builder &Texture::Builder::SetImage(Image *image)
{
    check(image);

    _image = image;

    return *this;
}

Texture::Builder &Texture::Builder::SetSampler(Sampler const *sampler)
{
    _sampler = sampler;

    return *this;
}

Texture::Builder &Texture::Builder::Format(vk::Format format)
{
    _format = format;

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

    _image = images.at(imageID);

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

Texture *Texture::Builder::Build(Device const *device) const
{
    check(_sampler);
    check(_image);

    return new Texture{device, _image, _sampler, _format, _depth, _name};
}

Texture::Texture(Device const *device, Image *image, Sampler const *sampler, vk::Format format, bool depth,
                 std::string const &name)
    : _name{name}, _image{image}, _sampler{sampler}, _ID{INVALID_ID}
{
    check(device);
    check(image->AskForVariant(format));

    vk::ImageViewCreateInfo createInfo;
    createInfo.viewType = vk::ImageViewType::e2D;
    createInfo.format = format;
    createInfo.image = image->GetImage(format);
    createInfo.subresourceRange.aspectMask = depth ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;

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

vk::Format Texture::GetFormat() const
{
    return _format;
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
