#include "fl_graph.h"
#include "fl_handles.h"
#include "wsp_device.h"

// lib
#include <spdlog/spdlog.h>
#include <variant>
#include <vulkan/vulkan_enums.hpp>

// std
#include <optional>
#include <stdexcept>

namespace fl
{

const size_t Graph::SAMPLER_DEPTH{0};
const size_t Graph::SAMPLER_COLOR_CLAMPED{1};
const size_t Graph::SAMPLER_COLOR_REPEATED{2};

Graph::Graph(const wsp::Device *device)
    : _passInfos{}, _resourceInfos{}, _targets{}, _images{}, _deviceMemories{}, _imageViews{}
{
    CreateSamplers(device);
}

void Graph::CreateSamplers(const wsp::Device *device)
{
    vk::SamplerCreateInfo samplerCreateInfo{};
    samplerCreateInfo.magFilter = vk::Filter::eLinear;
    samplerCreateInfo.minFilter = vk::Filter::eLinear;
    samplerCreateInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
    samplerCreateInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge;
    samplerCreateInfo.addressModeW = vk::SamplerAddressMode::eClampToEdge;
    samplerCreateInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
    samplerCreateInfo.unnormalizedCoordinates = false;
    samplerCreateInfo.compareEnable = ResourceRole::eDepth;

    _samplers[SAMPLER_DEPTH] = device->CreateSampler(samplerCreateInfo);

    samplerCreateInfo.compareEnable = ResourceRole::eDepth;

    _samplers[SAMPLER_COLOR_CLAMPED] = device->CreateSampler(samplerCreateInfo);

    samplerCreateInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
    samplerCreateInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
    samplerCreateInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
    samplerCreateInfo.compareEnable = ResourceRole::eColor;

    _samplers[SAMPLER_COLOR_REPEATED] = device->CreateSampler(samplerCreateInfo);
}

Graph::~Graph()
{
}

Resource Graph::NewResource(const ResourceCreateInfo &createInfo)
{
    _resourceInfos.push_back(createInfo);
    return Resource{_resourceInfos.size() - 1};
}

Pass Graph::NewPass(const PassCreateInfo &createInfo)
{
    _passInfos.push_back(createInfo);
    return Pass{_passInfos.size() - 1};
}

void Graph::FindDependencies(std::set<Resource> *validResources, std::set<Pass> *validPasses, Resource resource)
{
    validResources->emplace(resource);
    for (const Pass writer : _resourceInfos[resource.index].writers)
    {
        FindDependencies(validResources, validPasses, writer);
    }
}

void Graph::FindDependencies(std::set<Resource> *validResources, std::set<Pass> *validPasses, Pass pass)
{
    validPasses->emplace(pass);
    for (const Resource read : _passInfos[pass.index].reads)
    {
        FindDependencies(validResources, validPasses, read);
    }
}

void Graph::Compile(const wsp::Device *device)
{
    Reset(device);

    if (_targets.size() == 0)
    {
        spdlog::warn("Graph: no target resource selected");
        return;
    }

    for (size_t pass = 0; pass < _passInfos.size(); pass++)
    {
        for (const Resource resource : _passInfos[pass].reads)
        {
            _resourceInfos[resource.index].readers.push_back(Pass{pass});
        }
        for (const Resource resource : _passInfos[pass].writes)
        {
            _resourceInfos[resource.index].writers.push_back(Pass{pass});
        }
    }

    std::set<Resource> validResources;
    std::set<Pass> validPasses;

    for (const Resource target : _targets)
    {
        FindDependencies(&validResources, &validPasses, target);
    }

    for (const Resource resource : validResources)
    {
        Build(device, resource);
    }
    for (const Pass pass : validPasses)
    {
        Build(device, pass);
    }

    return;
}

void Graph::Run(vk::CommandBuffer commandBuffer)
{
    for (const std::function<void(vk::CommandBuffer)> &func : _orderedExecutes)
    {
        func(commandBuffer);
    }
}

void Graph::Reset(const wsp::Device *device)
{
    _targets.clear();

    Resource resource{0};

    for (ResourceCreateInfo &resourceInfo : _resourceInfos)
    {
        resourceInfo.writers.clear();
        resourceInfo.readers.clear();

        if (resourceInfo.isTarget)
        {
            _targets.push_back(resource);
        }

        resource.index++;
    }
}

void Graph::KhanFindOrder(const std::set<Resource> &resources, const std::set<Pass> &passes)
{
    _orderedExecutes.clear();

    std::map<std::variant<Resource, Pass>, int> degrees;

    for (const Resource resource : resources)
    {
        const ResourceCreateInfo &resourceInfo = _resourceInfos[resource.index];
        degrees[resource] = resourceInfo.writers.size();
    }

    for (const Pass pass : passes)
    {
        const PassCreateInfo &passInfo = _passInfos[pass.index];
        degrees[pass] = passInfo.reads.size();
    }

    for (auto [value, degree] : degrees)
    {
        if (degree == 0)
        {
            if (std::holds_alternative<Resource>(value))
            {
                const ResourceCreateInfo &resourceInfo = _resourceInfos[std::get<Resource>(value).index];
                for (Pass reader : resourceInfo.readers)
                {
                    degrees[reader]--;
                }
                degrees.erase(value);
            }
            if (std::holds_alternative<Pass>(value))
            {
                const PassCreateInfo &passInfo = _passInfos[std::get<Pass>(value).index];
                for (Resource writey : passInfo.writes)
                {
                    degrees[writey]--;
                }
                degrees.erase(value);
                _orderedExecutes.push_back(passInfo.execute);
            }
        }
    }

    if (degrees.size() > 0)
    {
        throw std::runtime_error("Graph: circular dependency found! cannot compile");
    }
}

void Graph::Build(const wsp::Device *device, Resource resource)
{
    const ResourceCreateInfo &createInfo = _resourceInfos[resource.index];
    vk::ImageCreateInfo imageInfo{};

    imageInfo.imageType = vk::ImageType::e2D;
    imageInfo.extent = vk::Extent3D{createInfo.extent.width, createInfo.extent.height, 1u};
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = vk::SampleCountFlagBits::e1;
    imageInfo.format = createInfo.format;
    imageInfo.tiling = vk::ImageTiling::eOptimal;
    imageInfo.initialLayout = vk::ImageLayout::eUndefined;

    if (createInfo.writers.size() > 0)
    {
        switch (createInfo.role)
        {
        case fl::ResourceRole::eColor:
            imageInfo.usage |= vk::ImageUsageFlagBits::eColorAttachment;
            break;
        case fl::ResourceRole::eDepth:
            imageInfo.usage |= vk::ImageUsageFlagBits::eDepthStencilAttachment;
            break;
        }
    }
    if (createInfo.readers.size() > 0)
    {
        imageInfo.usage |= vk::ImageUsageFlagBits::eSampled;
    }

    device->CreateImageAndBindMemory(imageInfo, &_images[resource], &_deviceMemories[resource]);

    vk::ImageViewCreateInfo viewInfo{};
    viewInfo.image = _images[resource];
    viewInfo.viewType = vk::ImageViewType::e2D;
    viewInfo.format = createInfo.format;
    viewInfo.subresourceRange.aspectMask =
        createInfo.role == ResourceRole::eDepth ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    device->CreateImageView(viewInfo, &_imageViews[resource]);

    // renderTarget.descriptorSetID =
    //    RenderSystem::getInstance()->createSamplerDescriptor(renderTarget.imageView, renderTarget.sampler);

    // vk::AttachmentDescription attachmentDescription{};
    // attachmentDescription.format = attachmentInfo.format;
    // attachmentDescription.samples = vk::SampleCountFlagBits::e1;
    // attachmentDescription.loadOp = attachmentInfo.loadOp;
    // attachmentDescription.storeOp = attachmentInfo.storeOp;
    // attachmentDescription.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    // attachmentDescription.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    // attachmentDescription.initialLayout = attachmentInfo.initial;
    // attachmentDescription.finalLayout = attachmentInfo.final;

    // attachmentDescriptions.push_back(attachmentDescription);
}

void Graph::Build(const wsp::Device *device, Pass pass)
{
    const PassCreateInfo &createInfo = _passInfos[pass.index];

    const vk::Extent2D outResolution{_resourceInfos[createInfo.writes[0].index].extent};
    std::vector<vk::ImageView> imageViews{};

    std::vector<vk::AttachmentReference> colorAttachmentReferences{};
    std::optional<vk::AttachmentReference> depthAttachmentReference{};
    colorAttachmentReferences.reserve(createInfo.writes.size());

    std::vector<vk::AttachmentDescription> attachmentDescriptions{};
    attachmentDescriptions.reserve(createInfo.writes.size());

    for (size_t i = 0; i < createInfo.writes.size(); i++)
    {
        const ResourceCreateInfo &resourceInfo = _resourceInfos[i];

        // check for image output mismatches
        if (outResolution != resourceInfo.extent)
        {
            std::ostringstream oss;
            oss << "Graph: resolution mismatch on pass " << pass.index << ", one w:" << outResolution.width
                << " h:" << outResolution.height << ", other w:" << resourceInfo.extent.width
                << " h:" << resourceInfo.extent.height;
            throw std::runtime_error(oss.str());
        }

        imageViews.push_back(_imageViews[Resource{i}]);

        vk::AttachmentDescription attachment = {};
        attachment.format = resourceInfo.format;
        attachment.samples = vk::SampleCountFlagBits::e1;
        attachment.loadOp = vk::AttachmentLoadOp::eClear;
        attachment.initialLayout = vk::ImageLayout::eUndefined;

        vk::AttachmentReference attachmentRef = {};
        attachmentRef.attachment = i;

        switch (resourceInfo.role)
        {
        case ResourceRole::eColor:
            attachment.storeOp = vk::AttachmentStoreOp::eStore;
            attachment.finalLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

            attachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

            colorAttachmentReferences.push_back(attachmentRef);
            break;
        case ResourceRole::eDepth:
            attachment.storeOp = vk::AttachmentStoreOp::eDontCare;
            attachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

            attachmentRef.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

            depthAttachmentReference = attachmentRef;
            break;
        }

        attachmentDescriptions.push_back(attachment);
    }

    vk::SubpassDescription subpass = {};
    subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = colorAttachmentReferences.data();
    subpass.pDepthStencilAttachment = &depthAttachmentReference.value();

    vk::RenderPassCreateInfo renderPassInfo{};
    renderPassInfo.attachmentCount = attachmentDescriptions.size();
    renderPassInfo.pAttachments = attachmentDescriptions.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    device->CreateRenderPass(renderPassInfo, &_renderPasses[pass]);

    vk::FramebufferCreateInfo framebufferInfo{};
    framebufferInfo.renderPass = _renderPasses[pass];
    framebufferInfo.attachmentCount = imageViews.size();
    framebufferInfo.pAttachments = imageViews.data();
    framebufferInfo.width = outResolution.width;
    framebufferInfo.height = outResolution.height;
    framebufferInfo.layers = 1;

    device->CreateFramebuffer(framebufferInfo, &_framebuffers[pass]);
}

} // namespace fl
