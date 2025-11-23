#include "wsp_constants.hpp"
#include <vulkan/vulkan_core.h>
#include <wsp_graph.hpp>

#include <wsp_device.hpp>
#include <wsp_devkit.hpp>
#include <wsp_engine.hpp>
#include <wsp_handles.hpp>
#include <wsp_render_manager.hpp>
#include <wsp_renderer.hpp>
#include <wsp_static_utils.hpp>
#include <wsp_swapchain.hpp>
#include <wsp_texture.hpp>

#include <client/TracyScoped.hpp>
#include <tracy/TracyC.h>

#include <spdlog/spdlog.h>

#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_structs.hpp>

#include <optional>
#include <stdexcept>
#include <unistd.h>
#include <variant>

using namespace wsp;

size_t const Graph::SAMPLER_DEPTH{0};
size_t const Graph::SAMPLER_COLOR_CLAMPED{1};
size_t const Graph::SAMPLER_COLOR_REPEATED{2};
size_t const Graph::MAX_SAMPLER_SETS{500};

void StaticTextureAllocator::BindStaticTexture(size_t id, class Texture const *texture) const
{
    check(texture);
    check(_device);

    vk::DescriptorImageInfo imageInfo{};
    imageInfo.sampler = _sampler;
    imageInfo.imageView = texture->GetImageView();
    imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

    vk::WriteDescriptorSet write{};
    write.dstSet = *_descriptorSet;
    write.dstArrayElement = id;
    write.descriptorCount = 1u;
    write.dstBinding = 0u;
    write.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    write.pImageInfo = &imageInfo;

    _device->UpdateDescriptorSets({write});
}

Graph::Graph(size_t width, size_t height)
    : _passInfos{}, _resourceInfos{}, _images{}, _deviceMemories{}, _imageViews{}, _framebuffers{}, _samplers{},
      _descriptorSets{}, _freed{false}, _target{0}, _width{width}, _height{height}, _uboSize{0}, _uboDescriptorSets{},
      _uboBuffers{}, _uboDeviceMemories{}
{
    BuildSamplers();
    BuildStaticTextures();
}

void Graph::BuildSamplers()
{
    Device const *device = SafeDeviceAccessor::Get();
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

    _samplers[SAMPLER_DEPTH] = device->CreateSampler(samplerCreateInfo, "depth sampler");

    samplerCreateInfo.compareEnable = false;

    _samplers[SAMPLER_COLOR_CLAMPED] = device->CreateSampler(samplerCreateInfo, "color sampler clamped");

    samplerCreateInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
    samplerCreateInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
    samplerCreateInfo.addressModeW = vk::SamplerAddressMode::eRepeat;

    _samplers[SAMPLER_COLOR_REPEATED] = device->CreateSampler(samplerCreateInfo, "color sampler repeated");

    vk::DescriptorPoolSize descriptorPoolSize{};
    descriptorPoolSize.descriptorCount = MAX_DYNAMIC_TEXTURES;
    descriptorPoolSize.type = vk::DescriptorType::eCombinedImageSampler;

    vk::DescriptorPoolCreateInfo descriptorPoolInfo{};
    descriptorPoolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
    descriptorPoolInfo.maxSets = MAX_SAMPLER_SETS;
    descriptorPoolInfo.poolSizeCount = 1u;
    descriptorPoolInfo.pPoolSizes = &descriptorPoolSize;

    device->CreateDescriptorPool(descriptorPoolInfo, &_descriptorPool, "graph descriptor pool");

    std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings{
        {0u, vk::DescriptorType::eCombinedImageSampler, 1u, {vk::ShaderStageFlagBits::eFragment}}};

    vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
    descriptorSetLayoutInfo.bindingCount = static_cast<uint32_t>(descriptorSetLayoutBindings.size());
    descriptorSetLayoutInfo.pBindings = descriptorSetLayoutBindings.data();

    device->CreateDescriptorSetLayout(descriptorSetLayoutInfo, &_descriptorSetLayout, "graph descriptor set layout");

    spdlog::debug("Graph: built samplers");
}

Graph::~Graph()
{
    if (!_freed)
    {
        spdlog::critical("Graph: forgot to Free before deletion");
    }
}

void Graph::Free()
{
    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    Reset();

    for (auto &[id, sampler] : _samplers)
    {
        device->DestroySampler(&sampler);
    }
    _samplers.clear();
    device->DestroyDescriptorSetLayout(&_descriptorSetLayout);
    device->DestroyDescriptorPool(&_descriptorPool);

    FreeStaticTextures();

    _freed = true;
    spdlog::info("Graph: freed");
}

Resource Graph::NewResource(ResourceCreateInfo const &createInfo)
{
    _resourceInfos.push_back(createInfo);
    return Resource{_resourceInfos.size() - 1};
}

Pass Graph::NewPass(PassCreateInfo const &createInfo)
{
    _passInfos.push_back(createInfo);
    return Pass{_passInfos.size() - 1};
}

StaticTextureAllocator Graph::GenerateStaticTextureAllocator(size_t const samplerID) const
{
    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    check(_samplers.size() > samplerID);

    return StaticTextureAllocator{_samplers.at(samplerID), &_staticTexturesDescriptorSet, device};
}

void Graph::SetUboSize(size_t const size)
{
    _uboSize = size;
}

void Graph::SetPopulateUboFunction(std::function<void *()> populateUbo)
{
    _populateUbo = populateUbo;
}

bool Graph::FindDependencies(std::set<Resource> *validResources, std::set<Pass> *validPasses, Resource resource,
                             std::set<std::variant<Resource, Pass>> &visitingStack)
{
    check(validResources);
    check(validPasses);

    if (visitingStack.find(resource) != visitingStack.end())
    {
        return true;
    }

    visitingStack.insert(resource);
    validResources->insert(resource);

    for (Pass const &writer : GetResourceInfo(resource).writers)
    {
        if (FindDependencies(validResources, validPasses, writer, visitingStack))
        {
            return true;
        }
    }

    visitingStack.erase(resource);
    return false;
}

bool Graph::FindDependencies(std::set<Resource> *validResources, std::set<Pass> *validPasses, Pass pass,
                             std::set<std::variant<Resource, Pass>> &visitingStack)
{
    check(validResources);
    check(validPasses);

    if (visitingStack.find(pass) != visitingStack.end())
    {
        return true;
    }

    visitingStack.insert(pass);
    validPasses->insert(pass);

    for (Resource const &write : GetPassInfo(pass).writes)
    {
        validResources->insert(write);
    }

    for (Resource const &read : GetPassInfo(pass).reads)
    {
        if (FindDependencies(validResources, validPasses, read, visitingStack))
        {
            return true;
        }
    }

    visitingStack.erase(pass);
    return false;
}

void Graph::Compile(Resource target, GraphUsage usage)
{
    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    _target = target;
    _usage = usage;

    Reset();

    for (size_t pass = 0; pass < _passInfos.size(); pass++)
    {
        PassCreateInfo const &passInfo = GetPassInfo(Pass{pass});

        if (!(passInfo.writes.empty() ||
              std::all_of(passInfo.writes.begin() + 1, passInfo.writes.end(), [&](Resource resource) {
                  return GetResourceInfo(resource).extent == GetResourceInfo(passInfo.writes[0]).extent;
              })))
        {
            std::ostringstream oss;
            oss << "Graph: resolution mismatch on pass " << pass;
            throw std::runtime_error(oss.str());
        }

        for (Resource const resource : GetPassInfo(Pass{pass}).reads)
        {
            check(_resourceInfos.size() > resource.index);
            _resourceInfos.at(resource.index).readers.push_back(Pass{pass});
        }
        for (Resource const resource : GetPassInfo(Pass{pass}).writes)
        {
            check(_resourceInfos.size() > resource.index);
            _resourceInfos.at(resource.index).writers.push_back(Pass{pass});
            _passInfos.at(pass).rebuildOnChange = true;
        }
    }

    _validPasses.clear();
    _validResources.clear();
    std::set<std::variant<Resource, Pass>> visitingStack{};

    if (FindDependencies(&_validResources, &_validPasses, target, visitingStack))
    {
        throw std::runtime_error("Graph: circular dependency found! cannot compile");
    }

    std::ostringstream oss;
    oss << "Graph: dependencies : ";
    for (Resource const resource : _validResources)
    {
        oss << GetResourceInfo(resource).debugName << ", ";
    }
    for (Pass const pass : _validPasses)
    {
        oss << GetPassInfo(pass).debugName << ", ";
    }
    spdlog::info("{0}", oss.str());

    KhanFindOrder(_validResources, _validPasses);

    _requestsUniform = false;

    for (Pass const pass : _validPasses)
    {
        if (GetPassInfo(pass).readsUniform)
        {
            if (_uboSize == 0)
            {
                throw std::runtime_error("Graph: Pass {0} reads uniform, but no uniform buffer size was set");
            }

            _requestsUniform = true;
        }
    }
    if (_requestsUniform)
    {
        BuildUbo();
    }

    for (Resource const resource : _validResources)
    {
        Build(resource);
    }
    for (Pass const pass : _validPasses)
    {
        Build(pass);
        BuildPipeline(pass);
    }

    spdlog::info("Graph: compiled");

    return;
}

vk::Image Graph::GetTargetImage() const
{
    if (_usage != GraphUsage::eToTransfer)
    {
        throw std::runtime_error("Graph: usage must be eToTransfer in order to get target image");
    }
    return _images.at(_target)[_currentFrameIndex];
}

vk::DescriptorSet Graph::GetTargetDescriptorSet() const
{
    if (_usage != GraphUsage::eToDescriptorSet)
    {
        throw std::runtime_error("Graph: usage must be eToDescriptorSet in order to get target descriptor set");
    }
    return _descriptorSets.at(_target)[_currentFrameIndex];
}

void Graph::ChangeUsage(GraphUsage usage)
{
    Compile(_target, usage);
}

void Graph::BuildPipeline(Pass pass)
{
    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    PassCreateInfo const &passInfo = GetPassInfo(pass);

    std::vector<char> const vertCode = ReadShaderFile(passInfo.vertFile);
    std::vector<char> const fragCode = ReadShaderFile(passInfo.fragFile);

    PipelineHolder &pipelineHolder = _pipelines[pass];
    device->CreateShaderModule(vertCode, &pipelineHolder.vertShaderModule,
                               passInfo.debugName + " vertex shader module");
    device->CreateShaderModule(fragCode, &pipelineHolder.fragShaderModule,
                               passInfo.debugName + " fragment shader module");

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
    viewportInfo.viewportCount = 1u;
    viewportInfo.pViewports = nullptr;
    viewportInfo.scissorCount = 1u;
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

    for (Resource const resource : passInfo.writes)
    {
        if (GetResourceInfo(resource).usage == ResourceUsage::eColor)
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
        if (GetResourceInfo(resource).usage == ResourceUsage::eDepth)
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
    dynamicStateInfo.pDynamicStates = dynamicStateEnables.data();
    dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
    dynamicStateInfo.flags = vk::PipelineDynamicStateCreateFlagBits{};

    std::vector<vk::DescriptorSetLayout> descriptorSetLayouts;

    if (passInfo.readsUniform)
    {
        descriptorSetLayouts.push_back(_uboDescriptorSetLayout);
    }
    for (size_t i = 0; i < passInfo.reads.size(); ++i)
    {
        descriptorSetLayouts.push_back(_descriptorSetLayout);
    }
    if (passInfo.readsStaticTextures)
    {
        descriptorSetLayouts.push_back(_staticTexturesDescriptorSetLayout);
    }

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = 0u;
    pipelineLayoutInfo.pPushConstantRanges = nullptr; // change with pushConstantRanges

    device->CreatePipelineLayout(pipelineLayoutInfo, &pipelineHolder.pipelineLayout,
                                 passInfo.debugName + " pipeline layout");

    vk::PushConstantRange pushConstantRange{};
    if (passInfo.pushConstantSize > 0)
    {
        pushConstantRange.size = passInfo.pushConstantSize;
        pushConstantRange.offset = 0u;
        pushConstantRange.stageFlags = vk::ShaderStageFlagBits::eAllGraphics;

        pipelineLayoutInfo.pushConstantRangeCount = 1u;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    }

    vk::GraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.stageCount = 2u;
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
    pipelineInfo.subpass = 0u;

    pipelineInfo.basePipelineIndex = -1;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    device->CreateGraphicsPipeline(pipelineInfo, &pipelineHolder.pipeline, passInfo.debugName + " graphics pipeline");

    spdlog::debug("Graph: built {0} pipeline", passInfo.debugName);
}

void Graph::FlushUbo(void *ubo, size_t frameIndex)
{
    if (!_requestsUniform)
    {
        return;
    }

    if (_uboSize == 0)
    {
        spdlog::warn("Graph: flushing ubo in a graph which doesn't include one");
        return;
    }
    check(ubo);

    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    memcpy(_uboMappedMemories[frameIndex], ubo, _uboSize);

    vk::MappedMemoryRange mappedRange = {};
    mappedRange.memory = _uboDeviceMemories[frameIndex];
    mappedRange.offset = 0u;
    mappedRange.size = _uboSize;

    device->FlushMappedMemoryRange(mappedRange);

    _currentFrameIndex = frameIndex;
}

void Graph::Render(vk::CommandBuffer commandBuffer, size_t frameIndex)
{
    extern TracyVkCtx TRACY_CTX;
    TracyVkZone(TRACY_CTX, commandBuffer, "graph");

    if (_populateUbo)
    {
        void *ubo = _populateUbo();
        FlushUbo(ubo, frameIndex);
    }

    for (Pass const pass : _orderedPasses)
    {
        vk::RenderPassBeginInfo renderPassInfo{};
        renderPassInfo.renderPass = _renderPasses.at(pass);
        renderPassInfo.framebuffer = _framebuffers.at(pass)[_currentFrameIndex];

        renderPassInfo.renderArea.offset = vk::Offset2D{0, 0};
        renderPassInfo.renderArea.extent = vk::Extent2D{uint32_t(_width), (uint32_t)_height};

        PassCreateInfo const &passInfo = GetPassInfo(pass);

        std::vector<vk::ClearValue> clearValues;
        clearValues.reserve(passInfo.writes.size());

        for (Resource const resource : passInfo.writes)
        {
            clearValues.push_back(GetResourceInfo(resource).clear);
        }
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

        extern TracyVkCtx TRACY_CTX;
        TracyVkZoneTransient(TRACY_CTX, __tracy, commandBuffer, passInfo.debugName.c_str(), true);

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

        size_t offset = 0;

        if (passInfo.readsUniform)
        {
            commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelines.at(pass).pipelineLayout,
                                             offset, 1u, &_uboDescriptorSets[_currentFrameIndex], 0u, nullptr);
            offset++;
        }
        if (passInfo.reads.size() > 0)
        {
            std::vector<vk::DescriptorSet> descriptorSets;
            descriptorSets.reserve(passInfo.reads.size());

            for (Resource const resource : passInfo.reads)
            {
                descriptorSets.push_back(_descriptorSets.at(resource)[_currentFrameIndex]);
            }

            commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelines.at(pass).pipelineLayout,
                                             offset, descriptorSets.size(), descriptorSets.data(), 0u, nullptr);

            offset += passInfo.reads.size();
        }

        if (passInfo.readsStaticTextures)
        {
            commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelines.at(pass).pipelineLayout,
                                             offset, 1u, &_staticTexturesDescriptorSet, 0u, nullptr);
        }

        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _pipelines.at(pass).pipeline);
        passInfo.execute(commandBuffer);
        commandBuffer.endRenderPass();
    }
}

void Graph::Reset()
{
    spdlog::debug("Graph: resetting began...");

    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    Resource resource{0};

    if (_uboDescriptorPool != VK_NULL_HANDLE)
    {
        FreeUbo();
    }

    for (ResourceCreateInfo &resourceInfo : _resourceInfos)
    {
        resourceInfo.writers.clear();
        resourceInfo.readers.clear();

        resource.index++;
    }

    for (Resource const resource : _validResources)
    {
        Free(resource);
    }
    for (Pass const pass : _validPasses)
    {
        Free(pass);
    }

    _descriptorSets.clear();
    _pipelines.clear();
    _images.clear();
    _deviceMemories.clear();
    _imageViews.clear();
    _framebuffers.clear();
    _renderPasses.clear();

    spdlog::debug("Graph: resetting complete...");
}

void Graph::Free(Resource resource)
{
    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    check(_validResources.find(resource) != _validResources.end());

    if (_descriptorSets.find(resource) != _descriptorSets.end())
    {
        for (vk::DescriptorSet &descriptorSet : _descriptorSets.at(resource))
        {
            device->FreeDescriptorSet(_descriptorPool, &descriptorSet);
        }
        _descriptorSets.erase(resource);
    }

    check(_images.find(resource) != _images.end());
    for (vk::Image &image : _images.at(resource))
    {
        device->DestroyImage(&image);
    }
    _images.erase(resource);

    check(_deviceMemories.find(resource) != _deviceMemories.end());
    for (vk::DeviceMemory &deviceMemory : _deviceMemories.at(resource))
    {
        device->FreeDeviceMemory(&deviceMemory);
    }
    _deviceMemories.erase(resource);

    check(_imageViews.find(resource) != _imageViews.end());
    for (vk::ImageView &imageView : _imageViews.at(resource))
    {
        device->DestroyImageView(&imageView);
    }
    _imageViews.erase(resource);

    ResourceCreateInfo createInfo = GetResourceInfo(resource);
    spdlog::debug("Graph: freed resource '{}'", createInfo.debugName);
}

void Graph::Free(Pass pass)
{
    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    check(_validPasses.find(pass) != _validPasses.end());

    PipelineHolder &pipelineHolder = _pipelines.at(pass);
    device->DestroyShaderModule(&pipelineHolder.vertShaderModule);
    device->DestroyShaderModule(&pipelineHolder.fragShaderModule);
    device->DestroyPipelineLayout(&pipelineHolder.pipelineLayout);
    device->DestroyGraphicsPipeline(&pipelineHolder.pipeline);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vk::Framebuffer &framebuffer = _framebuffers.at(pass)[i];
        device->DestroyFramebuffer(&framebuffer);
    }

    vk::RenderPass &renderPass = _renderPasses.at(pass);
    device->DestroyRenderPass(&renderPass);

    PassCreateInfo createInfo = GetPassInfo(pass);
    spdlog::debug("Graph: freed pass '{}'", createInfo.debugName);
}

void Graph::KhanFindOrder(std::set<Resource> const &resources, std::set<Pass> const &passes)
{
    _orderedPasses.clear();

    std::map<Resource, int> resourceDegrees{};
    std::map<Pass, int> passDegrees{};

    for (Resource const resource : resources)
    {
        ResourceCreateInfo const &resourceInfo = GetResourceInfo(resource);
        resourceDegrees[resource] = resourceInfo.writers.size();
    }

    for (Pass const pass : passes)
    {
        PassCreateInfo const &passInfo = GetPassInfo(pass);
        passDegrees[pass] = passInfo.reads.size();
    }

    while (passDegrees.size() > 0 && resourceDegrees.size() > 0)
    {
        bool progress = false;

        for (auto it = resourceDegrees.begin(); it != resourceDegrees.end();)
        {
            if (it->second == 0)
            {
                ResourceCreateInfo const &resourceInfo = GetResourceInfo(it->first);
                for (Pass const reader : resourceInfo.readers)
                {
                    passDegrees.at(reader)--;
                }
                it = resourceDegrees.erase(it);
                progress = true;
            }
            else
            {
                it++;
            }
        }
        for (auto it = passDegrees.begin(); it != passDegrees.end();)
        {
            if (it->second == 0)
            {
                PassCreateInfo const &passInfo = GetPassInfo(it->first);
                for (Resource const writey : passInfo.writes)
                {
                    resourceDegrees.at(writey)--;
                }
                _orderedPasses.push_back(it->first);
                it = passDegrees.erase(it);
                progress = true;
            }
            else
            {
                it++;
            }
        }
        if (progress == false)
        {
            std::ostringstream oss;
            oss << "Graph: circular dependency found! cannot compile: ";
            for (auto const &[resource, degree] : resourceDegrees)
            {
                ResourceCreateInfo const &resourceInfo = GetResourceInfo(resource);
                oss << resourceInfo.debugName << ", ";
            }
            for (auto const &[pass, degree] : passDegrees)
            {
                PassCreateInfo const &passInfo = GetPassInfo(pass);
                oss << passInfo.debugName << ", ";
            }
            throw std::runtime_error(oss.str());
        }
    }

    std::ostringstream oss;
    oss << "Graph: pass order selected: ";
    for (Pass const pass : _orderedPasses)
    {
        oss << " -> " << GetPassInfo(pass).debugName;
    }
    spdlog::info("{0}", oss.str());
}

void Graph::Build(Resource resource)
{
    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    ResourceCreateInfo const &createInfo = GetResourceInfo(resource);

    vk::ImageCreateInfo imageInfo{};
    imageInfo.imageType = vk::ImageType::e2D;
    imageInfo.mipLevels = 1u;
    imageInfo.arrayLayers = 1u;
    imageInfo.samples = vk::SampleCountFlagBits::e1;
    imageInfo.format = createInfo.format;
    imageInfo.tiling = vk::ImageTiling::eOptimal;
    imageInfo.initialLayout = vk::ImageLayout::eUndefined;
    imageInfo.mipLevels = 1u;
    imageInfo.samples = vk::SampleCountFlagBits::e1;
    imageInfo.arrayLayers = 1u;

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
        switch (createInfo.usage)
        {
        case ResourceUsage::eColor:
            imageInfo.usage |= vk::ImageUsageFlagBits::eColorAttachment;
            break;
        case ResourceUsage::eDepth:
            imageInfo.usage |= vk::ImageUsageFlagBits::eDepthStencilAttachment;
            break;
        }
    }
    if (IsSampled(resource))
    {
        imageInfo.usage |= vk::ImageUsageFlagBits::eSampled;
    }
    else
    {
        if (resource == _target)
        {
            imageInfo.usage |= vk::ImageUsageFlagBits::eTransferSrc;
        }
    }

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vk::Image image;
        vk::DeviceMemory deviceMemory;
        device->CreateImageAndBindMemory(imageInfo, &image, &deviceMemory, createInfo.debugName + " image");

        _images[resource][i] = image;
        _deviceMemories[resource][i] = deviceMemory;

        vk::ImageViewCreateInfo viewInfo{};
        viewInfo.image = _images.at(resource)[i];
        viewInfo.viewType = vk::ImageViewType::e2D;
        viewInfo.format = createInfo.format;
        viewInfo.subresourceRange.aspectMask = createInfo.usage == ResourceUsage::eDepth
                                                   ? vk::ImageAspectFlagBits::eDepth
                                                   : vk::ImageAspectFlagBits::eColor;
        viewInfo.subresourceRange.baseMipLevel = 0u;
        viewInfo.subresourceRange.levelCount = 1u;
        viewInfo.subresourceRange.baseArrayLayer = 0u;
        viewInfo.subresourceRange.layerCount = 1u;

        vk::ImageView imageView;
        device->CreateImageView(viewInfo, &imageView, createInfo.debugName + " image view");
        _imageViews[resource][i] = imageView;
    }

    if (IsSampled(resource))
    {
        BuildDescriptors(resource);
    }

    spdlog::debug("Graph: built {0} resource", createInfo.debugName);
}

void Graph::Build(Pass pass)
{
    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    PassCreateInfo const &createInfo = GetPassInfo(pass);

    vk::Extent2D outResolution{(uint32_t)_width, (uint32_t)_height};

    std::vector<vk::AttachmentReference> colorAttachmentReferences{};
    std::optional<vk::AttachmentReference> depthAttachmentReference{};
    colorAttachmentReferences.reserve(createInfo.writes.size());

    std::vector<vk::AttachmentDescription> attachmentDescriptions{};
    attachmentDescriptions.reserve(createInfo.writes.size());

    int i = 0;
    for (Resource const resource : createInfo.writes)
    {
        ResourceCreateInfo const &resourceInfo = GetResourceInfo(resource);

        if (resourceInfo.extent != vk::Extent2D{0, 0})
        {
            outResolution = resourceInfo.extent;
        }

        vk::AttachmentDescription attachment = {};
        attachment.format = resourceInfo.format;
        attachment.samples = vk::SampleCountFlagBits::e1;
        attachment.loadOp = vk::AttachmentLoadOp::eClear;
        attachment.initialLayout = vk::ImageLayout::eUndefined;

        vk::AttachmentReference attachmentRef = {};
        attachmentRef.attachment = i;

        switch (resourceInfo.usage)
        {
        case ResourceUsage::eColor:
            attachment.storeOp = vk::AttachmentStoreOp::eStore;
            attachment.finalLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

            attachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

            colorAttachmentReferences.push_back(attachmentRef);
            break;
        case ResourceUsage::eDepth:
            attachment.storeOp = vk::AttachmentStoreOp::eDontCare;
            attachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

            attachmentRef.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

            depthAttachmentReference = attachmentRef;
            break;
        }

        if (resource == _target && _usage == GraphUsage::eToTransfer)
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
    device->CreateRenderPass(renderPassInfo, &renderPass, createInfo.debugName + " render pass");
    _renderPasses[pass] = renderPass;

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        std::vector<vk::ImageView> imageViews{};
        for (Resource const resource : createInfo.writes)
        {
            check(_imageViews.find(resource) != _imageViews.end());
            imageViews.push_back(_imageViews.at(resource)[i]);
        }

        vk::FramebufferCreateInfo framebufferInfo{};
        framebufferInfo.renderPass = _renderPasses.at(pass);
        framebufferInfo.attachmentCount = imageViews.size();
        framebufferInfo.pAttachments = imageViews.data();
        framebufferInfo.width = outResolution.width;
        framebufferInfo.height = outResolution.height;
        framebufferInfo.layers = 1;

        vk::Framebuffer framebuffer;
        device->CreateFramebuffer(framebufferInfo, &framebuffer, createInfo.debugName + " framebuffer");
        _framebuffers[pass][i] = framebuffer;
    }

    spdlog::debug("Graph: built {0} pass", createInfo.debugName);
}

void Graph::BuildUbo()
{
    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    check(_uboSize != 0);
    std::vector<vk::DescriptorPoolSize> descriptorPoolSizes = {
        {vk::DescriptorType::eUniformBuffer, MAX_FRAMES_IN_FLIGHT}};

    vk::DescriptorPoolCreateInfo descriptorPoolInfo{};
    descriptorPoolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
    descriptorPoolInfo.maxSets = MAX_FRAMES_IN_FLIGHT;
    descriptorPoolInfo.poolSizeCount = (uint32_t)descriptorPoolSizes.size();
    descriptorPoolInfo.pPoolSizes = descriptorPoolSizes.data();

    device->CreateDescriptorPool(descriptorPoolInfo, &_uboDescriptorPool, "graph ubo descriptor pool");

    std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings{
        {0, vk::DescriptorType::eUniformBuffer, 1, {vk::ShaderStageFlagBits::eAllGraphics}}};

    vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
    descriptorSetLayoutInfo.bindingCount = static_cast<uint32_t>(descriptorSetLayoutBindings.size());
    descriptorSetLayoutInfo.pBindings = descriptorSetLayoutBindings.data();

    device->CreateDescriptorSetLayout(descriptorSetLayoutInfo, &_uboDescriptorSetLayout,
                                      "graph ubo descriptor set layout");

    std::vector<vk::WriteDescriptorSet> writeDescriptors{};
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vk::Buffer uboBuffer;
        vk::DeviceMemory uboDeviceMemory;
        void *uboMappedMemories = nullptr;

        vk::BufferCreateInfo createInfo;
        createInfo.size = _uboSize;
        createInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer;

        device->CreateBufferAndBindMemory(createInfo, &uboBuffer, &uboDeviceMemory,
                                          {vk::MemoryPropertyFlagBits::eHostVisible},
                                          "ubo buffer " + std::to_string(i));

        _uboBuffers[i] = uboBuffer;
        _uboDeviceMemories[i] = uboDeviceMemory;

        device->MapMemory(uboDeviceMemory, &uboMappedMemories);

        _uboMappedMemories[i] = uboMappedMemories;

        vk::DescriptorSet descriptorSet;

        vk::DescriptorSetAllocateInfo setAllocInfo{};
        setAllocInfo.descriptorPool = _uboDescriptorPool;
        setAllocInfo.descriptorSetCount = 1u;
        setAllocInfo.pSetLayouts = &_uboDescriptorSetLayout;

        device->AllocateDescriptorSet(setAllocInfo, &descriptorSet, "ubo descriptor set " + std::to_string(i));

        vk::DescriptorBufferInfo bufferInfo{};
        bufferInfo.offset = 0u;
        bufferInfo.buffer = uboBuffer;
        bufferInfo.range = _uboSize;

        vk::WriteDescriptorSet writeDescriptor{};
        writeDescriptor.dstSet = descriptorSet;
        writeDescriptor.dstBinding = 0u;
        writeDescriptor.dstArrayElement = 0u;
        writeDescriptor.descriptorType = vk::DescriptorType::eUniformBuffer;
        writeDescriptor.descriptorCount = 1u;
        writeDescriptor.pBufferInfo = &bufferInfo;

        writeDescriptors.push_back(writeDescriptor);

        _uboDescriptorSets[i] = descriptorSet;
    }

    device->UpdateDescriptorSets(writeDescriptors);

    spdlog::debug("Graph: built ubo");
}

void Graph::FreeUbo()
{
    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    for (vk::DescriptorSet &uboDescriptorSet : _uboDescriptorSets)
    {
        device->FreeDescriptorSet(_uboDescriptorPool, &uboDescriptorSet);
    }
    device->DestroyDescriptorPool(&_uboDescriptorPool);
    device->DestroyDescriptorSetLayout(&_uboDescriptorSetLayout);

    for (vk::DeviceMemory &deviceMemory : _uboDeviceMemories)
    {
        device->UnmapMemory(deviceMemory);
        device->FreeDeviceMemory(&deviceMemory);
    }
    for (vk::Buffer &buffer : _uboBuffers)
    {
        device->DestroyBuffer(&buffer);
    }

    spdlog::debug("Graph: Freed ubo");
}

void Graph::BuildStaticTextures()
{
    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    vk::DescriptorPoolSize staticTexturesDescriptorPoolSize{};
    staticTexturesDescriptorPoolSize.descriptorCount = MAX_STATIC_TEXTURES;
    staticTexturesDescriptorPoolSize.type = vk::DescriptorType::eCombinedImageSampler;

    vk::DescriptorPoolCreateInfo staticTexturesDescriptorPoolInfo{};
    staticTexturesDescriptorPoolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
    staticTexturesDescriptorPoolInfo.maxSets = MAX_SAMPLER_SETS;
    staticTexturesDescriptorPoolInfo.poolSizeCount = 1u;
    staticTexturesDescriptorPoolInfo.pPoolSizes = &staticTexturesDescriptorPoolSize;

    device->CreateDescriptorPool(staticTexturesDescriptorPoolInfo, &_staticTexturesDescriptorPool,
                                 "static textures descriptor pool");

    vk::DescriptorSetLayoutBinding descriptorSetLayoutBinding{};
    descriptorSetLayoutBinding.descriptorCount = MAX_STATIC_TEXTURES;
    descriptorSetLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eAllGraphics;
    descriptorSetLayoutBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;

    vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
    descriptorSetLayoutInfo.bindingCount = 1u;
    descriptorSetLayoutInfo.pBindings = &descriptorSetLayoutBinding;
    descriptorSetLayoutInfo.pNext = nullptr;

    device->CreateDescriptorSetLayout(descriptorSetLayoutInfo, &_staticTexturesDescriptorSetLayout,
                                      "static textures descriptor set layout");

    check(_samplers.find(SAMPLER_COLOR_CLAMPED) != _samplers.end());

    vk::DescriptorSetAllocateInfo setAllocInfo{};
    setAllocInfo.descriptorPool = _staticTexturesDescriptorPool;
    setAllocInfo.descriptorSetCount = 1u;
    setAllocInfo.pSetLayouts = &_staticTexturesDescriptorSetLayout;

    device->AllocateDescriptorSet(setAllocInfo, &_staticTexturesDescriptorSet, "static textures descriptor set");

    BuildDummyImage();

    vk::DescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    imageInfo.imageView = _dummyImageView;
    imageInfo.sampler = _samplers.at(SAMPLER_COLOR_CLAMPED);

    std::array<vk::DescriptorImageInfo, MAX_STATIC_TEXTURES> imageInfos{};
    imageInfos.fill(imageInfo);

    vk::WriteDescriptorSet writeDescriptor{};
    writeDescriptor.dstSet = _staticTexturesDescriptorSet;
    writeDescriptor.dstBinding = 0u;
    writeDescriptor.dstArrayElement = 0u;
    writeDescriptor.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    writeDescriptor.descriptorCount = MAX_SAMPLER_SETS;
    writeDescriptor.pImageInfo = imageInfos.data();

    device->UpdateDescriptorSets({writeDescriptor});

    spdlog::debug("Graph: built static textures");
}

void Graph::FreeStaticTextures()
{
    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    device->FreeDescriptorSet(_staticTexturesDescriptorPool, &_staticTexturesDescriptorSet);
    device->DestroyDescriptorPool(&_staticTexturesDescriptorPool);
    device->DestroyDescriptorSetLayout(&_staticTexturesDescriptorSetLayout);

    FreeDummyImage();

    spdlog::debug("Graph: freed static textures");
}

void Graph::BuildDummyImage()
{
    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    // Create image
    vk::ImageCreateInfo imageInfo{};
    imageInfo.imageType = vk::ImageType::e2D;
    imageInfo.extent = vk::Extent3D{1u, 1u, 1u};
    imageInfo.format = vk::Format::eR8G8B8Srgb;
    imageInfo.usage = vk::ImageUsageFlagBits::eSampled;
    imageInfo.initialLayout = vk::ImageLayout::eUndefined;
    imageInfo.tiling = vk::ImageTiling::eOptimal;
    imageInfo.mipLevels = 1u;
    imageInfo.arrayLayers = 1u;

    device->CreateImageAndBindMemory(imageInfo, &_dummyImage, &_dummyImageMemory, "Texture");

    // Create buffer and copy memory
    vk::BufferCreateInfo bufferInfo{};
    bufferInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
    bufferInfo.size = 1u;

    vk::Buffer buffer;
    vk::DeviceMemory deviceMemory;
    device->CreateBufferAndBindMemory(
        bufferInfo, &buffer, &deviceMemory,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, "Texture buffer");

    vk::ImageViewCreateInfo viewInfo{};
    viewInfo.image = _dummyImage;
    viewInfo.viewType = vk::ImageViewType::e2D;
    viewInfo.format = vk::Format::eR8G8B8Srgb;
    viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    device->CreateImageView(viewInfo, &_dummyImageView, "Texture image view");

    device->DestroyBuffer(&buffer);
    device->FreeDeviceMemory(&deviceMemory);

    vk::CommandBuffer const commandBuffer = device->BeginSingleTimeCommand();

    commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eAllCommands, {},
                                  {}, {},
                                  vk::ImageMemoryBarrier{vk::AccessFlagBits::eNone,
                                                         vk::AccessFlagBits::eShaderRead,
                                                         vk::ImageLayout::eUndefined,
                                                         vk::ImageLayout::eShaderReadOnlyOptimal,
                                                         vk::QueueFamilyIgnored,
                                                         vk::QueueFamilyIgnored,
                                                         _dummyImage,
                                                         {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}});

    device->EndSingleTimeCommand(commandBuffer);

    spdlog::debug("Graph: built dummy image");
}

void Graph::FreeDummyImage()
{
    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    device->DestroyImage(&_dummyImage);
    device->FreeDeviceMemory(&_dummyImageMemory);
    device->DestroyImageView(&_dummyImageView);

    spdlog::debug("Graph: freed dummy image");
}

void Graph::BuildDescriptors(Resource resource)
{
    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    ResourceCreateInfo const &resourceInfo = GetResourceInfo(resource);

    check(IsSampled(resource));
    check(_validResources.find(resource) != _validResources.end());
    check(_samplers.find(resourceInfo.sampler) != _samplers.end());

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vk::DescriptorSet descriptorSet;

        vk::DescriptorSetAllocateInfo setAllocInfo{};
        setAllocInfo.descriptorPool = _descriptorPool;
        setAllocInfo.descriptorSetCount = 1u;
        setAllocInfo.pSetLayouts = &_descriptorSetLayout;

        device->AllocateDescriptorSet(setAllocInfo, &descriptorSet, resourceInfo.debugName + " descriptor set");

        check(_imageViews.find(resource) != _imageViews.end());

        vk::DescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        imageInfo.imageView = _imageViews.at(resource)[i];
        imageInfo.sampler = _samplers.at(resourceInfo.sampler);

        vk::WriteDescriptorSet writeDescriptor{};
        writeDescriptor.dstSet = descriptorSet;
        writeDescriptor.dstBinding = 0u;
        writeDescriptor.dstArrayElement = 0u;
        writeDescriptor.descriptorType = vk::DescriptorType::eCombinedImageSampler;
        writeDescriptor.descriptorCount = 1u;
        writeDescriptor.pImageInfo = &imageInfo;

        device->UpdateDescriptorSets({writeDescriptor});

        _descriptorSets[resource][i] = descriptorSet;
    }

    spdlog::debug("Graph: built descriptors for '{}'", resourceInfo.debugName);
}

void Graph::Resize(size_t width, size_t height)
{
    spdlog::debug("Graph: resizing from {}x{} to {}x{}", _width, _height, width, height);

    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    _width = width;
    _height = height;

    std::vector<Resource> resourcesToRebuild;
    resourcesToRebuild.reserve(_validResources.size());
    std::vector<Pass> passesToRebuild;
    passesToRebuild.reserve(_validPasses.size());

    for (Resource const resource : _validResources)
    {
        ResourceCreateInfo const &resourceInfo = GetResourceInfo(resource);

        if (resourceInfo.extent == vk::Extent2D{0, 0})
        {
            Free(resource);
            resourcesToRebuild.push_back(resource);
        }
    }
    for (Pass const pass : _validPasses)
    {
        PassCreateInfo const &passInfo = GetPassInfo(pass);

        if (passInfo.rebuildOnChange)
        {
            Free(pass);
            passesToRebuild.push_back(pass);
        }
    }

    for (Resource const resource : resourcesToRebuild)
    {
        Build(resource);
    }

    for (Pass const pass : passesToRebuild)
    {
        Build(pass);
        BuildPipeline(pass);
    }

    spdlog::debug("Graph: resized to {}x{}", _width, _height, width, height);
}

void Graph::OnResizeCallback(void *graph, size_t width, size_t height)
{
    check(graph);
    reinterpret_cast<Graph *>(graph)->Resize(width, height);
}

vk::Sampler Graph::GetSampler(size_t ID) const
{
    check(_samplers.find(ID) != _samplers.end());

    return _samplers.at(ID);
}

bool Graph::IsSampled(Resource resource)
{
    if (_usage == eToTransfer)
    {
        return GetResourceInfo(resource).readers.size() > 0 && resource != _target;
    }
    return GetResourceInfo(resource).readers.size() > 0 || resource == _target;
}

PassCreateInfo const &Graph::GetPassInfo(Pass pass) const
{
    check(_passInfos.size() > pass.index);
    return _passInfos.at(pass.index);
}

ResourceCreateInfo const &Graph::GetResourceInfo(Resource resource) const
{
    check(_resourceInfos.size() > resource.index);
    return _resourceInfos.at(resource.index);
}
