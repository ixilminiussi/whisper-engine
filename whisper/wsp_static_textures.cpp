#include <wsp_static_textures.hpp>

#include <wsp_device.hpp>
#include <wsp_image.hpp>
#include <wsp_sampler.hpp>
#include <wsp_texture.hpp>

using namespace wsp;

StaticTextures::StaticTextures(size_t size, std::string const &name) : _name{name}, _size{size}, _offset{0u}
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

    device->CreateDescriptorPool(descriptorPoolInfo, &_descriptorPool, name + std::string("_descriptor_pool"));

    vk::DescriptorSetLayoutBinding descriptorSetLayoutBinding{};
    descriptorSetLayoutBinding.descriptorCount = size;
    descriptorSetLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eAllGraphics;
    descriptorSetLayoutBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;

    vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
    descriptorSetLayoutInfo.bindingCount = 1u;
    descriptorSetLayoutInfo.pBindings = &descriptorSetLayoutBinding;
    descriptorSetLayoutInfo.pNext = nullptr;

    device->CreateDescriptorSetLayout(descriptorSetLayoutInfo, &_descriptorSetLayout,
                                      name + std::string("_descriptor_set_layout"));

    vk::DescriptorSetAllocateInfo setAllocInfo{};
    setAllocInfo.descriptorPool = _descriptorPool;
    setAllocInfo.descriptorSetCount = 1u;
    setAllocInfo.pSetLayouts = &_descriptorSetLayout;

    device->AllocateDescriptorSet(setAllocInfo, &_descriptorSet, name + std::string("_descriptor_set"));

    BuildDummy();
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

    delete _dummyTexture;
    delete _dummyImage;
    delete _dummySampler;

    spdlog::info("StaticTextures: freed");
}

void StaticTextures::BuildDummy()
{
    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    _dummySampler = Sampler::Builder{}.Name(_name + "_dummy_sampler").Build(device);
    void *pixels = malloc(sizeof(char));
    _dummyImage = new Image{device, (char *)pixels, 1u, 1u, 3u, _name + std::string("_dummy_image")};

    _dummyTexture = Texture::Builder{}.SetImage(_dummyImage).SetSampler(_dummySampler).Build(device);

    spdlog::debug("Graph: built dummy image");
}

void StaticTextures::Clear()
{
    std::vector<Texture *> textures{};

    textures.resize(_size, _dummyTexture);

    _offset = 0u;

    Push(textures);

    _offset = 0u;
}

void StaticTextures::Push(std::vector<Texture *> const &textures)
{
    check(_offset + textures.size() <= _size);

    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    std::vector<vk::DescriptorImageInfo> imageInfos{};
    for (int i = 0; i < textures.size(); i++)
    {
        vk::DescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        imageInfo.imageView = textures[i]->GetImageView();
        imageInfo.sampler = textures[i]->GetSampler();

        textures[i]->SetID(_offset + i);

        imageInfos.push_back(imageInfo);
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
