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
    delete _dummyImage;
    delete _dummyTexture;

    spdlog::info("StaticTextures: freed");
}

void StaticTextures::BuildDummy(bool cubemap)
{
    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    void *pixels = malloc(sizeof(char));
    std::filesystem::path filepath;
    if (cubemap)
    {
        filepath = (std::filesystem::path(WSP_EDITOR_ASSETS) / std::filesystem::path("skybox.png")).lexically_normal();
    }
    else
    {
        filepath = (std::filesystem::path(WSP_EDITOR_ASSETS) / std::filesystem::path("missing-texture.png"))
                       .lexically_normal();
    }

    Image::CreateInfo imageCreateInfo{};
    imageCreateInfo.filepath = filepath;
    imageCreateInfo.format = vk::Format::eR8G8B8Srgb;
    imageCreateInfo.cubemap = cubemap;

    _dummyImage = new Image{device, imageCreateInfo};
    _dummySampler = new Sampler{device, Sampler::CreateInfo{}};

    Texture::CreateInfo createInfo{};
    createInfo.pImage = _dummyImage;
    createInfo.pSampler = _dummySampler;
    createInfo.format = vk::Format::eR8G8B8Srgb;
    createInfo.cubemap = cubemap;

    _dummyTexture = new Texture{device, createInfo};

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
            imageInfo.imageView = _dummyTexture->GetImageView();
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
