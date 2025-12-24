#include "wsp_assets_manager.hpp"
#include <fcntl.h>
#include <filesystem>
#include <wsp_static_textures.hpp>

#include <wsp_device.hpp>
#include <wsp_image.hpp>
#include <wsp_sampler.hpp>
#include <wsp_texture.hpp>

using namespace wsp;

StaticTextures::StaticTextures(uint32_t size, bool cubemap, std::string const &name)
    : _name{name}, _size{size}, _offset{0u}, _descriptorSet{}, _descriptorPool{}
{
    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    vk::DescriptorPoolSize descriptorPoolSize{};
    descriptorPoolSize.descriptorCount = size;
    descriptorPoolSize.type = vk::DescriptorType::eCombinedImageSampler;

    vk::DescriptorPoolCreateInfo descriptorPoolInfo{};
    descriptorPoolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
    descriptorPoolInfo.maxSets = size;
    descriptorPoolInfo.poolSizeCount = 1u;
    descriptorPoolInfo.pPoolSizes = &descriptorPoolSize;

    device->CreateDescriptorPool(descriptorPoolInfo, &_descriptorPool, fmt::format("{}<descriptor_pool>", name));

    vk::DescriptorSetLayoutBinding descriptorSetLayoutBinding{};
    descriptorSetLayoutBinding.descriptorCount = size;
    descriptorSetLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eAllGraphics;
    descriptorSetLayoutBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;

    vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
    descriptorSetLayoutInfo.bindingCount = 1u;
    descriptorSetLayoutInfo.pBindings = &descriptorSetLayoutBinding;
    descriptorSetLayoutInfo.pNext = nullptr;

    device->CreateDescriptorSetLayout(descriptorSetLayoutInfo, &_descriptorSetLayout,
                                      fmt::format("{}<descriptor_set_layout>", name));

    vk::DescriptorSetAllocateInfo setAllocInfo{};
    setAllocInfo.descriptorPool = _descriptorPool;
    setAllocInfo.descriptorSetCount = 1u;
    setAllocInfo.pSetLayouts = &_descriptorSetLayout;

    device->AllocateDescriptorSet(setAllocInfo, &_descriptorSet, fmt::format("{}<descriptor_set>", name));

    BuildDummy(cubemap);
    Clear();

    spdlog::debug("Graph: built static textures");
}

StaticTextures::~StaticTextures()
{
    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    device->FreeDescriptorSet(_descriptorPool, &_descriptorSet);
    device->DestroyDescriptorPool(&_descriptorPool);
    device->DestroyDescriptorSetLayout(&_descriptorSetLayout);

    delete _dummySampler;

    for (Image *image : _dummyImages)
    {
        delete image;
    }
    _dummyImages.clear();

    for (Texture *texture : _dummyTextures)
    {
        delete texture;
    }
    _dummyTextures.clear();

    spdlog::info("StaticTextures: freed");
}

void StaticTextures::BuildDummy(bool cubemap)
{
    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    _dummySampler = new Sampler{device, Sampler::CreateInfo{}};

    if (cubemap)
    {
        {
            Image::CreateInfo imageCreateInfo{};
            imageCreateInfo.filepath =
                (std::filesystem::path(WSP_EDITOR_ASSETS) / std::filesystem::path("skybox.exr")).lexically_normal();
            Image *image = new Image{device, imageCreateInfo, true};

            Texture::CreateInfo createInfo{};
            createInfo.pImage = image;
            createInfo.pSampler = _dummySampler;
            createInfo.cubemap = true;
            createInfo.name = "skybox";

            Texture *texture = new Texture{device, createInfo};

            _dummyImages.push_back(image);
            _dummyTextures.push_back(texture);
        }

        {
            Image::CreateInfo imageCreateInfo{};
            imageCreateInfo.filepath =
                (std::filesystem::path(WSP_EDITOR_ASSETS) / std::filesystem::path("irradiance.exr")).lexically_normal();
            Image *image = new Image{device, imageCreateInfo, true};

            Texture::CreateInfo createInfo{};
            createInfo.pImage = image;
            createInfo.pSampler = _dummySampler;
            createInfo.cubemap = true;
            createInfo.name = "irradiance";

            Texture *texture = new Texture{device, createInfo};

            _dummyImages.push_back(image);
            _dummyTextures.push_back(texture);
        }
    }
    else
    {
        Image::CreateInfo imageCreateInfo{};
        imageCreateInfo.filepath =
            (std::filesystem::path(WSP_EDITOR_ASSETS) / std::filesystem::path("missing-texture.png"))
                .lexically_normal();
        Image *image = new Image{device, imageCreateInfo, false};

        Texture::CreateInfo createInfo{};
        createInfo.pImage = image;
        createInfo.pSampler = _dummySampler;
        createInfo.name = "missing";
        createInfo.cubemap = false;

        Texture *texture = new Texture{device, createInfo};

        _dummyImages.push_back(image);
        _dummyTextures.push_back(texture);
    }

    spdlog::debug("Graph: built dummy texture");
}

void StaticTextures::Clear()
{
    std::vector<TextureID> textures{};

    textures.resize(_size, 0);

    _offset = 0u;

    Push(textures);

    _offset = 0u;
}

void StaticTextures::Push(std::vector<TextureID> const &textures)
{
    check(_offset + textures.size() <= _size);

    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    std::vector<vk::DescriptorImageInfo> imageInfos{};
    for (int i = 0; i < textures.size(); i++)
    {
        if (textures[i] == 0)
        {
            vk::DescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
            imageInfo.imageView = _dummyTextures[i % _dummyTextures.size()]->GetImageView();
            imageInfo.sampler = _dummySampler->GetSampler();

            imageInfos.push_back(imageInfo);
        }
        else
        {
            Texture const *texture = AssetsManager::Get()->GetTexture(textures[i]);

            vk::DescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
            imageInfo.imageView = texture->GetImageView();
            imageInfo.sampler = texture->GetSampler();

            imageInfos.push_back(imageInfo);

            _staticTextures[textures[i]] = _offset + i;
        }
    }

    vk::WriteDescriptorSet writeDescriptor{};
    writeDescriptor.dstSet = _descriptorSet;
    writeDescriptor.dstBinding = 0u;
    writeDescriptor.dstArrayElement = _offset;
    writeDescriptor.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    writeDescriptor.descriptorCount = imageInfos.size();
    writeDescriptor.pImageInfo = imageInfos.data();

    device->UpdateDescriptorSets({writeDescriptor});

    _offset += imageInfos.size();
}

vk::DescriptorSetLayout StaticTextures::GetDescriptorSetLayout() const
{
    return _descriptorSetLayout;
}

vk::DescriptorSet StaticTextures::GetDescriptorSet() const
{
    return _descriptorSet;
}

int StaticTextures::GetID(TextureID textureID) const
{
    if (textureID == 0)
    {
        return INVALID_ID;
    }

    check(_staticTextures.find(textureID) != _staticTextures.end());

    return (int)_staticTextures.at(textureID);
}
