#include <wsp_sampler.hpp>

#include <wsp_device.hpp>
#include <wsp_devkit.hpp>

#include <cgltf.h>
#include <spdlog/spdlog.h>

using namespace wsp;

Sampler::CreateInfo Sampler::GetCreateInfoFromGlTF(cgltf_sampler const *sampler)
{
    CreateInfo createInfo{};

    check(sampler);

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

    SetFilter(sampler->min_filter, &createInfo.minFilter, &createInfo.mipmapMode);
    SetFilter(sampler->min_filter, &createInfo.magFilter, &createInfo.mipmapMode);

    SetAddressMode(sampler->wrap_s, &createInfo.addressModeU);
    SetAddressMode(sampler->wrap_t, &createInfo.addressModeV);

    return createInfo;
}

Sampler::Sampler(Device const *device, Sampler::CreateInfo const &createInfo)
{
    check(device);

    vk::SamplerCreateInfo samplerCreateInfo{};
    samplerCreateInfo.magFilter = createInfo.magFilter;
    samplerCreateInfo.minFilter = createInfo.minFilter;
    samplerCreateInfo.addressModeU = createInfo.addressModeU;
    samplerCreateInfo.addressModeV = createInfo.addressModeV;
    samplerCreateInfo.addressModeW = createInfo.addressModeW;
    samplerCreateInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
    samplerCreateInfo.unnormalizedCoordinates = false;
    samplerCreateInfo.compareEnable = createInfo.depth;
    samplerCreateInfo.mipmapMode = createInfo.mipmapMode;
    samplerCreateInfo.minLod = 0.f;
    samplerCreateInfo.maxLod = 7.f;

    _sampler = device->CreateSampler(samplerCreateInfo, "sampler");
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
