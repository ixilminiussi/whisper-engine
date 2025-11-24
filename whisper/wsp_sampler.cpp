#include <wsp_sampler.hpp>

#include <wsp_device.hpp>
#include <wsp_devkit.hpp>

#include <cgltf.h>
#include <spdlog/spdlog.h>

using namespace wsp;

Sampler::Builder::Builder()
{
    _createInfo.magFilter = vk::Filter::eLinear;
    _createInfo.minFilter = vk::Filter::eLinear;
    _createInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
    _createInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge;
    _createInfo.addressModeW = vk::SamplerAddressMode::eClampToEdge;
    _createInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
    _createInfo.unnormalizedCoordinates = false;
    _createInfo.compareEnable = false;
    _createInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
}

Sampler::Builder &Sampler::Builder::AddressMode(vk::SamplerAddressMode addressMode)
{
    _createInfo.addressModeU = addressMode;
    _createInfo.addressModeV = addressMode;
    _createInfo.addressModeW = addressMode;

    return *this;
}

Sampler::Builder &Sampler::Builder::Depth()
{
    _createInfo.compareEnable = true;

    return *this;
}

Sampler::Builder &Sampler::Builder::Name(std::string const &name)
{
    _name = name;

    return *this;
}

Sampler::Builder &Sampler::Builder::GlTF(cgltf_sampler const *sampler)
{
    check(sampler);

    _name = std::string(sampler->name);

    auto SetFilter = [](cgltf_filter_type type, vk::Filter *filter, vk::SamplerMipmapMode *mipMapMode) {
        check(filter);
        check(mipMapMode);

        switch (type)
        {
        case cgltf_filter_type_undefined:
            break;
        case cgltf_filter_type_nearest:
            *filter = vk::Filter::eNearest;
            break;
        case cgltf_filter_type_linear:
            *filter = vk::Filter::eLinear;
            break;
        case cgltf_filter_type_nearest_mipmap_nearest:
            *filter = vk::Filter::eNearest;
            *mipMapMode = vk::SamplerMipmapMode::eNearest;
            break;
        case cgltf_filter_type_linear_mipmap_nearest:
            *filter = vk::Filter::eLinear;
            *mipMapMode = vk::SamplerMipmapMode::eNearest;
            break;
        case cgltf_filter_type_nearest_mipmap_linear:
            *filter = vk::Filter::eNearest;
            *mipMapMode = vk::SamplerMipmapMode::eLinear;
            break;
        case cgltf_filter_type_linear_mipmap_linear:
            *filter = vk::Filter::eLinear;
            *mipMapMode = vk::SamplerMipmapMode::eLinear;
            break;
        }
    };

    auto SetAddressMode = [](cgltf_wrap_mode mode, vk::SamplerAddressMode *addressMode) {
        check(addressMode);

        switch (mode)
        {
        case cgltf_wrap_mode_clamp_to_edge:
            *addressMode = vk::SamplerAddressMode::eClampToEdge;
            break;
        case cgltf_wrap_mode_mirrored_repeat:
            *addressMode = vk::SamplerAddressMode::eMirroredRepeat;
            break;
        case cgltf_wrap_mode_repeat:
            *addressMode = vk::SamplerAddressMode::eRepeat;
            break;
        }
    };

    SetFilter(sampler->min_filter, &_createInfo.minFilter, &_createInfo.mipmapMode);
    SetFilter(sampler->min_filter, &_createInfo.magFilter, &_createInfo.mipmapMode);

    SetAddressMode(sampler->wrap_s, &_createInfo.addressModeU);
    SetAddressMode(sampler->wrap_t, &_createInfo.addressModeV);

    return *this;
}

Sampler *Sampler::Builder::Build(Device const *device)
{
    check(device);

    return new Sampler(device, _createInfo, _name);
}

Sampler::Sampler(Device const *device, vk::SamplerCreateInfo const &createInfo, std::string const &name) : _name{name}
{
    check(device);

    _sampler = device->CreateSampler(createInfo, _name + std::string("_sampler"));
}

Sampler::~Sampler()
{
    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    device->DestroySampler(&_sampler);

    spdlog::debug("Sampler: freed");
}

vk::Sampler Sampler::GetSampler() const
{
    return _sampler;
}
