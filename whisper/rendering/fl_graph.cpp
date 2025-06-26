#include "fl_graph.h"
#include "fl_handles.h"
#include "wsp_device.h"
#include "wsp_devkit.h"
#include "wsp_static_utils.h"

// lib
#include <spdlog/spdlog.h>
#include <unistd.h>
#include <vulkan/vulkan_enums.hpp>

// std
#include <optional>
#include <stdexcept>
#include <variant>
#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_structs.hpp>

namespace fl
{

const size_t Graph::SAMPLER_DEPTH{0};
const size_t Graph::SAMPLER_COLOR_CLAMPED{1};
const size_t Graph::SAMPLER_COLOR_REPEATED{2};
const size_t Graph::MAX_SAMPLER_SETS{500};

Graph::Graph(const wsp::Device *device, size_t width, size_t height)
    : _passInfos{}, _resourceInfos{}, _images{}, _deviceMemories{}, _imageViews{}, _freed{false}, _target{0},
      _width{width}, _height{height}
{
    check(device);

    CreateSamplers(device);
}

void Graph::CreateSamplers(const wsp::Device *device)
{
    check(device);

    vk::SamplerCreateInfo samplerCreateInfo{};
    samplerCreateInfo.magFilter = vk::Filter::eLinear;
    samplerCreateInfo.minFilter = vk::Filter::eLinear;
    samplerCreateInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
    samplerCreateInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge;
    samplerCreateInfo.addressModeW = vk::SamplerAddressMode::eClampToEdge;
    samplerCreateInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
    samplerCreateInfo.unnormalizedCoordinates = false;
    samplerCreateInfo.compareEnable = true;

    _samplers[SAMPLER_DEPTH] = device->CreateSampler(samplerCreateInfo);

    samplerCreateInfo.compareEnable = false;

    _samplers[SAMPLER_COLOR_CLAMPED] = device->CreateSampler(samplerCreateInfo);

    samplerCreateInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
    samplerCreateInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
    samplerCreateInfo.addressModeW = vk::SamplerAddressMode::eRepeat;

    _samplers[SAMPLER_COLOR_REPEATED] = device->CreateSampler(samplerCreateInfo);

    std::vector<vk::DescriptorPoolSize> poolSizes = {{vk::DescriptorType::eCombinedImageSampler, 100}};

    vk::DescriptorPoolCreateInfo poolInfo{};
    poolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
    poolInfo.maxSets = MAX_SAMPLER_SETS;
    poolInfo.poolSizeCount = (uint32_t)poolSizes.size();
    poolInfo.pPoolSizes = poolSizes.data();

    device->CreateDescriptorPool(poolInfo, &_samplerDescriptorPool);

    std::vector<vk::DescriptorSetLayoutBinding> setLayoutBindings{
        {0, vk::DescriptorType::eCombinedImageSampler, 1, {vk::ShaderStageFlagBits::eFragment}}};

    vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
    descriptorSetLayoutInfo.sType = vk::StructureType::eDescriptorSetLayoutCreateInfo;
    descriptorSetLayoutInfo.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
    descriptorSetLayoutInfo.pBindings = setLayoutBindings.data();

    device->CreateDescriptorSetLayout(descriptorSetLayoutInfo, &_samplerDescriptorSetLayout);
}

Graph::~Graph()
{
    if (!_freed)
    {
        spdlog::critical("Graph: forgot to Free before deletion");
    }
}

void Graph::Free(const wsp::Device *device)
{
    check(device);

    for (auto &[id, sampler] : _samplers)
    {
        device->DestroySampler(sampler);
    }
    _samplers.clear();
    device->DestroyDescriptorSetLayout(_samplerDescriptorSetLayout);
    device->DestroyDescriptorPool(_samplerDescriptorPool);

    Reset(device);

    _freed = true;
    spdlog::info("Graph: succesfully freed");
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
    for (const Pass writer : GetResourceInfo(resource).writers)
    {
        FindDependencies(validResources, validPasses, writer);
    }
}

void Graph::FindDependencies(std::set<Resource> *validResources, std::set<Pass> *validPasses, Pass pass)
{
    validPasses->emplace(pass);
    for (const Resource write : GetPassInfo(pass).writes)
    {
        validResources->emplace(write);
    }
    for (const Resource read : GetPassInfo(pass).reads)
    {
        FindDependencies(validResources, validPasses, read);
    }
}

void Graph::Compile(const wsp::Device *device, Resource target)
{
    check(device);

    Reset(device);

    _target = target;

    for (size_t pass = 0; pass < _passInfos.size(); pass++)
    {
        const PassCreateInfo &passInfo = GetPassInfo(Pass{pass});
        if (!(passInfo.writes.empty() ||
              std::all_of(passInfo.writes.begin() + 1, passInfo.writes.end(), [&](Resource resource) {
                  return GetResourceInfo(resource).extent == GetResourceInfo(passInfo.writes[0]).extent;
              })))
        {
            std::ostringstream oss;
            oss << "Graph: resolution mismatch on pass " << pass;
            throw std::runtime_error(oss.str());
        }

        for (const Resource resource : GetPassInfo(Pass{pass}).reads)
        {
            check(_resourceInfos.size() > resource.index);
            _resourceInfos.at(resource.index).readers.push_back(Pass{pass});
        }
        for (const Resource resource : GetPassInfo(Pass{pass}).writes)
        {
            check(_resourceInfos.size() > resource.index);
            _resourceInfos.at(resource.index).writers.push_back(Pass{pass});
            _passInfos.at(pass).rebuildOnChange = true;
        }
    }

    _validPasses.clear();
    _validResources.clear();

    FindDependencies(&_validResources, &_validPasses, target);

    for (const Resource resource : _validResources)
    {
        Build(device, resource);
    }
    for (const Pass pass : _validPasses)
    {
        Build(device, pass);
        CreatePipeline(device, pass);
    }

    KhanFindOrder(_validResources, _validPasses);

    spdlog::info("Graph: finished compilation");

    return;
}

vk::Image Graph::GetTargetImage()
{
    return _images[_target];
}

void Graph::CreatePipeline(const wsp::Device *device, Pass pass)
{
    check(device);
    const PassCreateInfo &passInfo = GetPassInfo(pass);

    const std::vector<char> vertCode = wsp::ReadShaderFile(passInfo.vertFile);
    const std::vector<char> fragCode = wsp::ReadShaderFile(passInfo.fragFile);

    PipelineHolder &pipelineHolder = _pipelines[pass];
    device->CreateShaderModule(vertCode, &pipelineHolder.vertShaderModule);
    device->CreateShaderModule(fragCode, &pipelineHolder.fragShaderModule);

    vk::PipelineShaderStageCreateInfo shaderStages[2];
    shaderStages[0].stage = vk::ShaderStageFlagBits::eVertex;
    shaderStages[0].module = pipelineHolder.vertShaderModule;
    shaderStages[0].pName = "main";
    shaderStages[0].flags = vk::PipelineShaderStageCreateFlagBits{};
    shaderStages[0].pNext = nullptr;
    shaderStages[0].pSpecializationInfo = nullptr;
    shaderStages[1].stage = vk::ShaderStageFlagBits::eFragment;
    shaderStages[1].module = pipelineHolder.fragShaderModule;
    shaderStages[1].pName = "main";
    shaderStages[1].flags = vk::PipelineShaderStageCreateFlagBits{};
    shaderStages[1].pNext = nullptr;
    shaderStages[1].pSpecializationInfo = nullptr;

    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
    inputAssemblyInfo.topology = vk::PrimitiveTopology::eTriangleList;
    inputAssemblyInfo.primitiveRestartEnable = vk::False;

    vk::PipelineViewportStateCreateInfo viewportInfo{};
    viewportInfo.viewportCount = 1;
    viewportInfo.pViewports = nullptr;
    viewportInfo.scissorCount = 1;
    viewportInfo.pScissors = nullptr;

    vk::PipelineRasterizationStateCreateInfo rasterizationInfo{};
    rasterizationInfo.depthClampEnable = vk::False;
    rasterizationInfo.rasterizerDiscardEnable = vk::False;
    rasterizationInfo.polygonMode = vk::PolygonMode::eFill;
    rasterizationInfo.lineWidth = 1.0f;
    rasterizationInfo.cullMode = vk::CullModeFlagBits::eFront;
    rasterizationInfo.frontFace = vk::FrontFace::eClockwise;
    rasterizationInfo.depthBiasEnable = vk::False;
    rasterizationInfo.depthBiasConstantFactor = 0.0f;
    rasterizationInfo.depthBiasClamp = 0.0f;
    rasterizationInfo.depthBiasSlopeFactor = 0.0f;

    vk::PipelineMultisampleStateCreateInfo multisampleInfo{};
    multisampleInfo.sampleShadingEnable = vk::False;
    multisampleInfo.rasterizationSamples = vk::SampleCountFlagBits::e1;
    multisampleInfo.minSampleShading = 1.0f;
    multisampleInfo.pSampleMask = nullptr;
    multisampleInfo.alphaToCoverageEnable = vk::False;
    multisampleInfo.alphaToOneEnable = vk::False;

    vk::PipelineDepthStencilStateCreateInfo depthStencilInfo{};
    depthStencilInfo.depthTestEnable = vk::False;
    depthStencilInfo.depthWriteEnable = vk::False;
    depthStencilInfo.depthCompareOp = vk::CompareOp::eAlways;
    depthStencilInfo.depthBoundsTestEnable = vk::False;
    depthStencilInfo.minDepthBounds = 0.0f;
    depthStencilInfo.maxDepthBounds = 1.0f;
    depthStencilInfo.stencilTestEnable = vk::False;
    depthStencilInfo.front = vk::StencilOpState{};
    depthStencilInfo.back = vk::StencilOpState{};

    std::vector<vk::PipelineColorBlendAttachmentState> colorBlendAttachments{};

    for (const Resource resource : passInfo.writes)
    {
        if (GetResourceInfo(resource).role == ResourceRole::eColor)
        {
            vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
            colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                                  vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
            colorBlendAttachment.blendEnable = vk::False;
            colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eOne;
            colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eZero;
            colorBlendAttachment.colorBlendOp = vk::BlendOp::eAdd;
            colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
            colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
            colorBlendAttachment.alphaBlendOp = vk::BlendOp::eAdd;

            colorBlendAttachments.push_back(colorBlendAttachment);
        }
        if (GetResourceInfo(resource).role == ResourceRole::eDepth)
        {
            depthStencilInfo.depthTestEnable = vk::True;
            depthStencilInfo.depthWriteEnable = vk::True;
            depthStencilInfo.depthCompareOp = vk::CompareOp::eLess;
        }
    }

    vk::PipelineColorBlendStateCreateInfo colorBlendInfo{};
    colorBlendInfo.logicOpEnable = vk::False;
    colorBlendInfo.logicOp = vk::LogicOp::eCopy;
    colorBlendInfo.attachmentCount = static_cast<uint32_t>(colorBlendAttachments.size());
    colorBlendInfo.pAttachments = colorBlendAttachments.data();
    colorBlendInfo.blendConstants[0] = 0.0f;
    colorBlendInfo.blendConstants[1] = 0.0f;
    colorBlendInfo.blendConstants[2] = 0.0f;
    colorBlendInfo.blendConstants[3] = 0.0f;

    vk::PipelineDynamicStateCreateInfo dynamicStateInfo{};
    std::vector<vk::DynamicState> dynamicStateEnables = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
    dynamicStateInfo.sType = vk::StructureType::ePipelineDynamicStateCreateInfo;
    dynamicStateInfo.pDynamicStates = dynamicStateEnables.data();
    dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
    dynamicStateInfo.flags = vk::PipelineDynamicStateCreateFlagBits{};

    const std::vector<vk::DescriptorSetLayout> descriptorSetLayouts(passInfo.reads.size(), _samplerDescriptorSetLayout);

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = vk::StructureType::ePipelineLayoutCreateInfo;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr; // change with pushConstantRanges

    device->CreatePipelineLayout(pipelineLayoutInfo, &pipelineHolder.pipelineLayout);

    vk::GraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = vk::StructureType::eGraphicsPipelineCreateInfo;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &passInfo.vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
    pipelineInfo.pViewportState = &viewportInfo;
    pipelineInfo.pRasterizationState = &rasterizationInfo;
    pipelineInfo.pMultisampleState = &multisampleInfo;
    pipelineInfo.pColorBlendState = &colorBlendInfo;
    pipelineInfo.pDepthStencilState = &depthStencilInfo;
    pipelineInfo.pDynamicState = &dynamicStateInfo;

    pipelineInfo.layout = pipelineHolder.pipelineLayout;
    pipelineInfo.renderPass = _renderPasses.at(pass);
    pipelineInfo.subpass = 0;

    pipelineInfo.basePipelineIndex = -1;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    device->CreateGraphicsPipeline(pipelineInfo, &pipelineHolder.pipeline);
}

void Graph::Run(vk::CommandBuffer commandBuffer)
{
    for (const Pass pass : _orderedPasses)
    {
        vk::RenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = vk::StructureType::eRenderPassBeginInfo;
        renderPassInfo.renderPass = _renderPasses.at(pass);
        renderPassInfo.framebuffer = _framebuffers.at(pass);

        renderPassInfo.renderArea.offset = vk::Offset2D{0, 0};
        renderPassInfo.renderArea.extent = vk::Extent2D{uint32_t(_width), (uint32_t)_height};

        const PassCreateInfo &passInfo = GetPassInfo(pass);

        std::vector<vk::ClearValue> clearValues;
        clearValues.reserve(passInfo.writes.size());

        for (const Resource resource : passInfo.writes)
        {
            clearValues.push_back(GetResourceInfo(resource).clear);
        }
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

        static vk::Viewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(_width);
        viewport.height = static_cast<float>(_height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        static vk::Rect2D scissor{};
        scissor.offset = vk::Offset2D{0, 0};
        scissor.extent = vk::Extent2D{(uint32_t)_width, (uint32_t)_height};

        commandBuffer.setViewport(0, 1, &viewport);
        commandBuffer.setScissor(0, 1, &scissor);

        if (passInfo.reads.size() > 0)
        {
            std::vector<vk::DescriptorSet> samplerDescriptorSets;
            samplerDescriptorSets.reserve(passInfo.reads.size());

            for (const Resource resource : passInfo.reads)
            {
                samplerDescriptorSets.push_back(_samplerDescriptorSets.at(resource));
            }

            commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelines.at(pass).pipelineLayout, 0,
                                             samplerDescriptorSets.size(), samplerDescriptorSets.data(), 0, nullptr);
        }

        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _pipelines.at(pass).pipeline);
        passInfo.execute(commandBuffer);
        commandBuffer.endRenderPass();
    }
}

void Graph::Reset(const wsp::Device *device)
{
    check(device);

    Resource resource{0};

    for (ResourceCreateInfo &resourceInfo : _resourceInfos)
    {
        resourceInfo.writers.clear();
        resourceInfo.readers.clear();

        resource.index++;
    }

    for (const Resource resource : _validResources)
    {
        Free(device, resource);
    }
    for (const Pass pass : _validPasses)
    {
        Free(device, pass);
    }

    _pipelines.clear();
    _images.clear();
    _deviceMemories.clear();
    _imageViews.clear();
    _framebuffers.clear();
    _renderPasses.clear();
}

void Graph::Free(const wsp::Device *device, Resource resource)
{
    check(_validResources.find(resource) != _validResources.end());

    const vk::Image image = _images.at(resource);
    device->DestroyImage(image);

    const vk::DeviceMemory deviceMemory = _deviceMemories.at(resource);
    device->FreeDeviceMemory(deviceMemory);

    const vk::ImageView imageView = _imageViews.at(resource);
    device->DestroyImageView(imageView);
}

void Graph::Free(const wsp::Device *device, Pass pass)
{
    check(_validPasses.find(pass) != _validPasses.end());

    const PipelineHolder &pipelineHolder = _pipelines.at(pass);
    device->DestroyShaderModule(pipelineHolder.vertShaderModule);
    device->DestroyShaderModule(pipelineHolder.fragShaderModule);
    device->DestroyPipelineLayout(pipelineHolder.pipelineLayout);
    device->DestroyGraphicsPipeline(pipelineHolder.pipeline);

    const vk::Framebuffer framebuffer = _framebuffers.at(pass);
    device->DestroyFramebuffer(framebuffer);

    const vk::RenderPass renderPass = _renderPasses.at(pass);
    device->DestroyRenderPass(renderPass);
}

void Graph::KhanFindOrder(const std::set<Resource> &resources, const std::set<Pass> &passes)
{
    _orderedPasses.clear();

    std::map<std::variant<Resource, Pass>, int> degrees;

    for (const Resource resource : resources)
    {
        const ResourceCreateInfo &resourceInfo = GetResourceInfo(resource);
        degrees[resource] = resourceInfo.writers.size();
    }

    for (const Pass pass : passes)
    {
        const PassCreateInfo &passInfo = GetPassInfo(pass);
        degrees[pass] = passInfo.reads.size();
    }

    while (degrees.size() > 0)
    {
    restart:
        for (auto &[value, degree] : degrees)
        {
            if (degree == 0)
            {
                if (std::holds_alternative<Resource>(value))
                {
                    const ResourceCreateInfo &resourceInfo = GetResourceInfo(std::get<Resource>(value));
                    for (Pass reader : resourceInfo.readers)
                    {
                        degrees[reader]--;
                    }
                    degrees.erase(value);
                    goto restart;
                }
                if (std::holds_alternative<Pass>(value))
                {
                    const PassCreateInfo &passInfo = GetPassInfo(std::get<Pass>(value));
                    for (Resource writey : passInfo.writes)
                    {
                        degrees[writey]--;
                    }
                    degrees.erase(value);
                    _orderedPasses.push_back(std::get<Pass>(value));
                    goto restart;
                }
            }
        }
        if (degrees.size() > 0)
        {
            throw std::runtime_error("Graph: circular dependency found! cannot compile");
        }
    }
}

void Graph::Build(const wsp::Device *device, Resource resource)
{
    check(device);

    const ResourceCreateInfo &createInfo = GetResourceInfo(resource);
    vk::ImageCreateInfo imageInfo{};

    imageInfo.imageType = vk::ImageType::e2D;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = vk::SampleCountFlagBits::e1;
    imageInfo.format = createInfo.format;
    imageInfo.tiling = vk::ImageTiling::eOptimal;
    imageInfo.initialLayout = vk::ImageLayout::eUndefined;
    imageInfo.mipLevels = 1;
    imageInfo.samples = vk::SampleCountFlagBits::e1;
    imageInfo.arrayLayers = 1;

    if (createInfo.extent == vk::Extent2D{0, 0})
    {
        imageInfo.extent = vk::Extent3D{(uint32_t)_width, (uint32_t)_height, 1u};
    }
    else
    {
        imageInfo.extent = vk::Extent3D{createInfo.extent.width, createInfo.extent.height, 1u};
    }

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
    if (resource == _target)
    {
        imageInfo.usage |= vk::ImageUsageFlagBits::eTransferSrc;
    }

    vk::Image image;
    vk::DeviceMemory deviceMemory;
    device->CreateImageAndBindMemory(imageInfo, &image, &deviceMemory);
    _images[resource] = image;
    _deviceMemories[resource] = deviceMemory;

    vk::ImageViewCreateInfo viewInfo{};
    viewInfo.image = _images.at(resource);
    viewInfo.viewType = vk::ImageViewType::e2D;
    viewInfo.format = createInfo.format;
    viewInfo.subresourceRange.aspectMask =
        createInfo.role == ResourceRole::eDepth ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    vk::ImageView imageView;
    device->CreateImageView(viewInfo, &imageView);
    _imageViews[resource] = imageView;

    if (createInfo.readers.size() > 0)
    {
        CreateSamplerDescriptor(device, resource);
    }
}

void Graph::Build(const wsp::Device *device, Pass pass)
{
    check(device);

    int j = 0;
    const PassCreateInfo &createInfo = GetPassInfo(pass);

    vk::Extent2D outResolution{(uint32_t)_width, (uint32_t)_height};

    std::vector<vk::ImageView> imageViews{};

    std::vector<vk::AttachmentReference> colorAttachmentReferences{};
    std::optional<vk::AttachmentReference> depthAttachmentReference{};
    colorAttachmentReferences.reserve(createInfo.writes.size());

    std::vector<vk::AttachmentDescription> attachmentDescriptions{};
    attachmentDescriptions.reserve(createInfo.writes.size());

    int i = 0;
    for (const Resource resource : createInfo.writes)
    {
        const ResourceCreateInfo &resourceInfo = GetResourceInfo(resource);

        if (resourceInfo.extent != vk::Extent2D{0, 0})
        {
            outResolution = resourceInfo.extent;
        }

        imageViews.push_back(_imageViews.at(resource));

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

        if (resource == _target)
        {
            attachment.finalLayout = vk::ImageLayout::eTransferSrcOptimal;
        }

        attachmentDescriptions.push_back(attachment);
        i++;
    }

    vk::SubpassDescription subpass = {};
    subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = colorAttachmentReferences.data();
    subpass.pDepthStencilAttachment =
        depthAttachmentReference.has_value() ? &depthAttachmentReference.value() : nullptr;

    vk::RenderPassCreateInfo renderPassInfo{};
    renderPassInfo.attachmentCount = attachmentDescriptions.size();
    renderPassInfo.pAttachments = attachmentDescriptions.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    vk::RenderPass renderPass;
    device->CreateRenderPass(renderPassInfo, &renderPass);
    _renderPasses[pass] = renderPass;

    vk::FramebufferCreateInfo framebufferInfo{};
    framebufferInfo.renderPass = _renderPasses.at(pass);
    framebufferInfo.attachmentCount = imageViews.size();
    framebufferInfo.pAttachments = imageViews.data();
    framebufferInfo.width = outResolution.width;
    framebufferInfo.height = outResolution.height;
    framebufferInfo.layers = 1;

    vk::Framebuffer framebuffer;
    device->CreateFramebuffer(framebufferInfo, &framebuffer);
    _framebuffers[pass] = framebuffer;
}

void Graph::CreateSamplerDescriptor(const wsp::Device *device, Resource resource)
{
    const ResourceCreateInfo &resourceInfo = GetResourceInfo(resource);

    check(resourceInfo.readers.size() > 0);
    check(_validResources.find(resource) != _validResources.end());
    check(_samplers.find(resourceInfo.sampler) != _samplers.end());

    vk::DescriptorSet descriptorSet;

    vk::DescriptorSetAllocateInfo setAllocInfo{};
    setAllocInfo.descriptorPool = _samplerDescriptorPool;
    setAllocInfo.descriptorSetCount = 1;
    setAllocInfo.pSetLayouts = &(_samplerDescriptorSetLayout);

    device->AllocateDescriptorSet(setAllocInfo, &descriptorSet);

    vk::DescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    imageInfo.imageView = _imageViews.at(resource);
    imageInfo.sampler = _samplers.at(resourceInfo.sampler);

    vk::WriteDescriptorSet descriptorWrite{};
    descriptorWrite.dstSet = descriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfo;

    device->UpdateDescriptorSet(descriptorWrite);

    _samplerDescriptorSets[resource] = descriptorSet;
}

void Graph::WindowResizeCallback(void *graph, const wsp::Device *device, size_t width, size_t height)
{
    reinterpret_cast<Graph *>(graph)->OnResize(device, width, height);
}

void Graph::OnResize(const wsp::Device *device, size_t width, size_t height)
{
    _width = width;
    _height = height;

    std::vector<Resource> resourcesToRebuild;
    resourcesToRebuild.reserve(_validResources.size());
    std::vector<Pass> passesToRebuild;
    passesToRebuild.reserve(_validPasses.size());

    for (const Resource resource : _validResources)
    {
        const ResourceCreateInfo &resourceInfo = GetResourceInfo(resource);

        if (resourceInfo.extent == vk::Extent2D{0, 0})
        {
            Free(device, resource);
            resourcesToRebuild.push_back(resource);
        }
    }
    for (const Pass pass : _validPasses)
    {
        const PassCreateInfo &passInfo = GetPassInfo(pass);

        if (passInfo.rebuildOnChange)
        {
            Free(device, pass);
            passesToRebuild.push_back(pass);
        }
    }

    for (const Resource resource : resourcesToRebuild)
    {
        Build(device, resource);
    }

    for (const Pass pass : passesToRebuild)
    {
        Build(device, pass);
        CreatePipeline(device, pass);
    }
}

const PassCreateInfo &Graph::GetPassInfo(Pass pass) const
{
    check(_passInfos.size() > pass.index);
    return _passInfos.at(pass.index);
}

const ResourceCreateInfo &Graph::GetResourceInfo(Resource resource) const
{
    check(_resourceInfos.size() > resource.index);
    return _resourceInfos.at(resource.index);
}

} // namespace fl
