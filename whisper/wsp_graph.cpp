#include <wsp_graph.hpp>

#include <wsp_constants.hpp>
#include <wsp_device.hpp>
#include <wsp_devkit.hpp>
#include <wsp_engine.hpp>
#include <wsp_handles.hpp>
#include <wsp_image.hpp>
#include <wsp_render_manager.hpp>
#include <wsp_renderer.hpp>
#include <wsp_sampler.hpp>
#include <wsp_static_textures.hpp>
#include <wsp_static_utils.hpp>
#include <wsp_swapchain.hpp>
#include <wsp_texture.hpp>

#include <client/TracyScoped.hpp>
#include <tracy/TracyC.h>

#include <spdlog/spdlog.h>

#include <vulkan/vulkan.hpp>

#include <optional>
#include <stdexcept>
#include <unistd.h>
#include <variant>

using namespace wsp;

uint32_t const Graph::SAMPLER_DEPTH{0};
uint32_t const Graph::SAMPLER_COLOR_CLAMPED{1};
uint32_t const Graph::SAMPLER_COLOR_REPEATED{2};

Graph::Graph(uint32_t width, uint32_t height)
    : _passInfos{}, _resourceInfos{}, _images{}, _textures{}, _framebuffers{}, _descriptorSets{}, _target{0},
      _width{width}, _height{height}, _uboSize{0}, _uboDescriptorSets{}, _uboBuffers{}, _uboDeviceMemories{},
      _currentFrameIndex{0}
{
    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    BuildSamplers(device);
    BuildDescriptorPool();
}

void Graph::BuildSamplers(Device const *device)
{
    check(device);

    Sampler::CreateInfo createInfo{};
    createInfo.depth = true;

    _depthSampler = new Sampler{device, createInfo};

    createInfo.depth = false;

    _colorSampler = new Sampler{device, createInfo};

    spdlog::debug("Graph: built samplers");
}

void Graph::BuildDescriptorPool()
{
    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    vk::DescriptorPoolSize descriptorPoolSize{};
    descriptorPoolSize.descriptorCount = MAX_DYNAMIC_TEXTURES;
    descriptorPoolSize.type = vk::DescriptorType::eCombinedImageSampler;

    vk::DescriptorPoolCreateInfo descriptorPoolInfo{};
    descriptorPoolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
    descriptorPoolInfo.maxSets = MAX_DYNAMIC_TEXTURES;
    descriptorPoolInfo.poolSizeCount = 1u;
    descriptorPoolInfo.pPoolSizes = &descriptorPoolSize;

    device->CreateDescriptorPool(descriptorPoolInfo, &_descriptorPool, "_graph_descriptor_pool");

    std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings{
        {0u, vk::DescriptorType::eCombinedImageSampler, 1u, {vk::ShaderStageFlagBits::eFragment}}};

    vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
    descriptorSetLayoutInfo.bindingCount = static_cast<uint32_t>(descriptorSetLayoutBindings.size());
    descriptorSetLayoutInfo.pBindings = descriptorSetLayoutBindings.data();

    device->CreateDescriptorSetLayout(descriptorSetLayoutInfo, &_descriptorSetLayout, "_graph_descriptor_set_layout");

    spdlog::debug("Graph: built descriptor pool");
}

Graph::~Graph()
{
    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    Reset();

    delete _depthSampler;
    delete _colorSampler;

    device->DestroyDescriptorSetLayout(&_descriptorSetLayout);
    device->DestroyDescriptorPool(&_descriptorPool);

    spdlog::info("Graph: freed");
}

Resource Graph::NewResource(ResourceCreateInfo const &createInfo)
{
    _resourceInfos.push_back(createInfo);
    return Resource{(uint32_t)_resourceInfos.size() - 1};
}

Pass Graph::NewPass(PassCreateInfo const &createInfo)
{
    _passInfos.push_back(createInfo);
    return Pass{(uint32_t)_passInfos.size() - 1};
}

void Graph::SetUboSize(uint32_t const size)
{
    _uboSize = size;
}

void Graph::SetPopulateUboFunction(std::function<void *()> populateUbo)
{
    _populateUbo = populateUbo;
}

void Graph::FindDependencies(std::set<Resource> *validResources, std::set<Pass> *validPasses, Resource resource,
                             std::set<std::variant<Resource, Pass>> &visitingStack)
{
    check(validResources);
    check(validPasses);

    if (visitingStack.find(resource) != visitingStack.end())
    {
        return;
    }

    visitingStack.insert(resource);
    validResources->insert(resource);

    for (Pass const &writer : GetResourceInfo(resource).writers)
    {
        FindDependencies(validResources, validPasses, writer, visitingStack);
    }

    visitingStack.erase(resource);
}

void Graph::FindDependencies(std::set<Resource> *validResources, std::set<Pass> *validPasses, Pass pass,
                             std::set<std::variant<Resource, Pass>> &visitingStack)
{
    check(validResources);
    check(validPasses);

    if (visitingStack.find(pass) != visitingStack.end())
    {
        return;
    }

    visitingStack.insert(pass);
    validPasses->insert(pass);

    for (Resource const &write : GetPassInfo(pass).writes)
    {
        FindDependencies(validResources, validPasses, write, visitingStack);
    }

    for (Resource const &read : GetPassInfo(pass).reads)
    {
        FindDependencies(validResources, validPasses, read, visitingStack);
    }

    visitingStack.erase(pass);
}

void Graph::Compile(Resource target, GraphUsage usage)
{
    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    _target = target;
    _usage = usage;

    Reset();

    for (uint32_t pass = 0; pass < _passInfos.size(); pass++)
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

        for (Resource const resource : passInfo.reads)
        {
            check(_resourceInfos.size() > resource.index);
            _resourceInfos.at(resource.index).readers.push_back(Pass{pass});
        }
        for (Resource const resource : passInfo.writes)
        {
            check(_resourceInfos.size() > resource.index);
            _resourceInfos.at(resource.index).writers.push_back(Pass{pass});
            _passInfos.at(pass).rebuildOnChange = true;
        }
        for (Pass const dependency : passInfo.passDependencies)
        {
            check(_passInfos.size() > dependency.index);
            _passInfos.at(dependency.index).passDependants.push_back(Pass{pass});
        }
    }

    _validPasses.clear();
    _validResources.clear();
    std::set<std::variant<Resource, Pass>> visitingStack{};

    FindDependencies(&_validResources, &_validPasses, target, visitingStack);

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

Image *Graph::GetTargetImage() const
{
    if (_usage != GraphUsage::eToTransfer)
    {
        throw std::runtime_error("Graph: usage must be eToTransfer in order to get target image");
    }
    check(_images.find(_target) != _images.end());

    Image *image = _images.at(_target)[_currentFrameIndex];
    check(image);

    return image;
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
                               passInfo.debugName + "_vertex_shader_module");
    device->CreateShaderModule(fragCode, &pipelineHolder.fragShaderModule,
                               passInfo.debugName + "_fragment_shader_module");

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
    rasterizationInfo.cullMode = vk::CullModeFlagBits::eBack;
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
        ResourceCreateInfo const &resourceInfo = GetResourceInfo(resource);
        if (resourceInfo.usage == ResourceUsage::eColor)
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
        else if (resourceInfo.usage == ResourceUsage::eDepth)
        {
            depthStencilInfo.depthTestEnable = vk::True;
            depthStencilInfo.depthWriteEnable = vk::True;
            depthStencilInfo.depthCompareOp = vk::CompareOp::eLessOrEqual;
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
    for (uint32_t i = 0; i < passInfo.reads.size(); ++i)
    {
        descriptorSetLayouts.push_back(_descriptorSetLayout);
    }
    for (StaticTextures const *staticTextures : passInfo.staticTextures)
    {
        descriptorSetLayouts.push_back(staticTextures->GetDescriptorSetLayout());
    }

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = 0u;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;

    vk::PushConstantRange pushConstantRange{};
    if (passInfo.pushConstantSize > 0)
    {
        pushConstantRange.size = passInfo.pushConstantSize;
        pushConstantRange.offset = 0u;
        pushConstantRange.stageFlags = vk::ShaderStageFlagBits::eAllGraphics;

        pipelineLayoutInfo.pushConstantRangeCount = 1u;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    }

    device->CreatePipelineLayout(pipelineLayoutInfo, &pipelineHolder.pipelineLayout,
                                 passInfo.debugName + "<pipeline_layout>");

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

    device->CreateGraphicsPipeline(pipelineInfo, &pipelineHolder.pipeline, passInfo.debugName + "_graphics_pipeline");

    spdlog::debug("Graph: built {0} pipeline", passInfo.debugName);
}

void Graph::FlushUbo(void *ubo)
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

    memcpy(_uboMappedMemories[_currentFrameIndex], ubo, _uboSize);

    vk::MappedMemoryRange mappedRange = {};
    mappedRange.memory = _uboDeviceMemories[_currentFrameIndex];
    mappedRange.offset = 0u;
    mappedRange.size = _uboSize;

    device->FlushMappedMemoryRange(mappedRange);
}

void Graph::Render(vk::CommandBuffer commandBuffer, uint32_t frameIndex)
{
    _currentFrameIndex = 1;

    extern TracyVkCtx TRACY_CTX;
    TracyVkZone(TRACY_CTX, commandBuffer, "graph");

    if (_populateUbo)
    {
        void *ubo = _populateUbo();
        FlushUbo(ubo);
    }

    for (Pass const pass : _orderedPasses)
    {
        if (auto it = _imageMemoryBarriers.find(pass); it != _imageMemoryBarriers.end())
        {
            int i = 0;
            for (std::array<vk::ImageMemoryBarrier, MAX_FRAMES_IN_FLIGHT> const &barriers : it->second)
            {
                vk::ImageMemoryBarrier const &barrier = barriers[_currentFrameIndex];

                commandBuffer.pipelineBarrier(
                    vk::PipelineStageFlagBits::eLateFragmentTests | vk::PipelineStageFlagBits::eColorAttachmentOutput,
                    vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests,
                    vk::DependencyFlagBits::eByRegion, 0, nullptr, 0, nullptr, 1, &barrier);

                i++;
            }
        }

        PassCreateInfo const &passInfo = GetPassInfo(pass);

        vk::RenderPassBeginInfo renderPassInfo{};
        renderPassInfo.renderPass = _renderPasses.at(pass);
        renderPassInfo.framebuffer = _framebuffers.at(pass)[_currentFrameIndex];

        renderPassInfo.renderArea.offset = vk::Offset2D{0, 0};

        if (ResourceCreateInfo const &resourceInfo = GetResourceInfo(passInfo.writes.at(0));
            resourceInfo.extent != vk::Extent2D{0u, 0u})
        {
            renderPassInfo.renderArea.extent = resourceInfo.extent;
        }
        else
        {
            renderPassInfo.renderArea.extent = vk::Extent2D{uint32_t(_width), (uint32_t)_height};
        }

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
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        static vk::Rect2D scissor{};
        scissor.offset = vk::Offset2D{0, 0};

        if (ResourceCreateInfo const &resourceInfo = GetResourceInfo(passInfo.writes.at(0));
            resourceInfo.extent != vk::Extent2D{0u, 0u})
        {
            viewport.width = resourceInfo.extent.width; // WARN: important fix, need to send to tauri
            viewport.height = resourceInfo.extent.height;
            scissor.extent = resourceInfo.extent;
        }
        else
        {
            viewport.width = static_cast<float>(_width);
            viewport.height = static_cast<float>(_height);
            scissor.extent = vk::Extent2D{(uint32_t)_width, (uint32_t)_height};
        }

        commandBuffer.setViewport(0, 1, &viewport);
        commandBuffer.setScissor(0, 1, &scissor);

        uint32_t offset = 0;

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

        for (StaticTextures const *staticTextures : passInfo.staticTextures)
        {
            vk::DescriptorSet const staticDescriptorSet = staticTextures->GetDescriptorSet();
            commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelines.at(pass).pipelineLayout,
                                             offset, 1u, &staticDescriptorSet, 0u, nullptr);
            offset++;
        }

        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _pipelines.at(pass).pipeline);
        passInfo.execute(commandBuffer, _pipelines.at(pass).pipelineLayout);
        commandBuffer.endRenderPass();
    }
}

void Graph::Reset()
{
    spdlog::debug("Graph: resetting began...");

    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    if (_uboDescriptorPool != VK_NULL_HANDLE)
    {
        FreeUbo();
    }

    for (ResourceCreateInfo &resourceInfo : _resourceInfos)
    {
        resourceInfo.writers.clear();
        resourceInfo.readers.clear();
    }
    for (PassCreateInfo &passInfo : _passInfos)
    {
        passInfo.passDependants.clear();
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
    _textures.clear();
    _framebuffers.clear();
    _renderPasses.clear();
    _imageMemoryBarriers.clear();

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
    for (Image *image : _images.at(resource))
    {
        delete image;
    }
    _images.erase(resource);

    check(_textures.find(resource) != _textures.end());
    for (Texture *texture : _textures.at(resource))
    {
        delete texture;
    }
    _textures.erase(resource);

    ResourceCreateInfo createInfo = GetResourceInfo(resource);
    spdlog::debug("Graph: freed resource '{}'", createInfo.debugName);
}

void Graph::Free(Pass pass)
{
    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    auto it = _imageMemoryBarriers.find(pass);
    if (it != _imageMemoryBarriers.end())
    {
        _imageMemoryBarriers.erase(it);
    }

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
        passDegrees[pass] = passInfo.reads.size() + passInfo.passDependencies.size();
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
                for (Pass const dependant : passInfo.passDependants)
                {
                    passDegrees.at(dependant)--;
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
                oss << resourceInfo.debugName << ": " << degree << ", ";
            }
            for (auto const &[pass, degree] : passDegrees)
            {
                PassCreateInfo const &passInfo = GetPassInfo(pass);
                oss << passInfo.debugName << ": " << degree << ", ";
            }
            throw std::runtime_error(oss.str());
        }
    }

    std::ostringstream oss;
    oss << "Graph: pass order selected: ";
    for (int i = 0; i < _orderedPasses.size(); i++)
    {
        if (i != 0)
        {
            oss << " -> ";
        }
        oss << GetPassInfo(_orderedPasses[i]).debugName;
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
        _images[resource][i] = new Image(device, imageInfo, createInfo.debugName + std::string("_image"));

        Texture::CreateInfo textureCreateInfo{};
        textureCreateInfo.pImage = _images[resource][i];
        textureCreateInfo.name = createInfo.debugName;
        textureCreateInfo.pSampler = _colorSampler;

        if (createInfo.usage == ResourceUsage::eDepth)
        {
            textureCreateInfo.depth = true;
            textureCreateInfo.pSampler = _depthSampler;
        }

        _textures[resource][i] = new Texture(device, textureCreateInfo);
    }

    if (IsSampled(resource))
    {
        BuildDescriptors(resource);
    }

    spdlog::debug("Graph: built <{0}> resource", createInfo.debugName);
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

        // if we aren't the only one to write to a resource, instead of clearing it, we want to load it and create an
        // imagebarrier to ensure it is only cleared once at the start
        std::vector<Pass> orderedWriters;
        orderedWriters.reserve(resourceInfo.writers.size());

        for (Pass const potentialWriter : _orderedPasses)
        {
            if (std::find(resourceInfo.writers.begin(), resourceInfo.writers.end(), potentialWriter) !=
                resourceInfo.writers.end())
            {
                orderedWriters.push_back(potentialWriter);
            }
        }

        int pos = 0;
        for (Pass const writer : orderedWriters)
        {
            if (writer == pass)
            {
                break;
            }
            pos++;
        }

        // if we're not the first to write to a resource, we need to create a barrier to ensure the correct order of
        // operations
        if (pos != 0)
        {
            std::array<vk::ImageMemoryBarrier, MAX_FRAMES_IN_FLIGHT> imageMemoryBarriers;
            vk::ImageMemoryBarrier imageMemoryBarrier{};

            imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
            imageMemoryBarrier.subresourceRange.levelCount = 1;
            imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
            imageMemoryBarrier.subresourceRange.layerCount = 1;

            if (resourceInfo.usage == ResourceUsage::eDepth)
            {
                imageMemoryBarrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
                imageMemoryBarrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
                imageMemoryBarrier.oldLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
                imageMemoryBarrier.newLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
                imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
                imageMemoryBarrier.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
                imageMemoryBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
            }
            if (resourceInfo.usage == ResourceUsage::eColor)
            {
                imageMemoryBarrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
                imageMemoryBarrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
                imageMemoryBarrier.oldLayout = vk::ImageLayout::eColorAttachmentOptimal;
                imageMemoryBarrier.newLayout = vk::ImageLayout::eColorAttachmentOptimal;
                imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
                imageMemoryBarrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
                imageMemoryBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
            }

            for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
            {
                imageMemoryBarrier.image = _images.at(resource)[i]->GetImage();

                imageMemoryBarriers[i] = imageMemoryBarrier;
            }

            _imageMemoryBarriers[pass].push_back(imageMemoryBarriers);
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
            if (pos != 0)
            {
                attachment.loadOp = vk::AttachmentLoadOp::eLoad;
                attachment.initialLayout = vk::ImageLayout::eColorAttachmentOptimal;
            }
            attachment.storeOp = vk::AttachmentStoreOp::eStore;
            attachment.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;
            if (pos == orderedWriters.size() - 1 && IsSampled(resource))
            {
                attachment.finalLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
            }

            attachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

            colorAttachmentReferences.push_back(attachmentRef);
            break;
        case ResourceUsage::eDepth:
            if (pos != 0)
            {
                attachment.loadOp = vk::AttachmentLoadOp::eLoad;
                attachment.initialLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
            }
            attachment.storeOp = vk::AttachmentStoreOp::eDontCare;
            attachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
            if (pos == orderedWriters.size() - 1 && IsSampled(resource))
            {
                attachment.finalLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
            }

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
    subpass.colorAttachmentCount = colorAttachmentReferences.size();
    subpass.pColorAttachments = colorAttachmentReferences.data();
    subpass.pDepthStencilAttachment =
        depthAttachmentReference.has_value() ? &depthAttachmentReference.value() : nullptr;

    vk::RenderPassCreateInfo renderPassInfo{};
    renderPassInfo.attachmentCount = attachmentDescriptions.size();
    renderPassInfo.pAttachments = attachmentDescriptions.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    vk::RenderPass renderPass;
    device->CreateRenderPass(renderPassInfo, &renderPass, createInfo.debugName + "<render_pass>");
    _renderPasses[pass] = renderPass;

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        std::vector<vk::ImageView> imageViews{};
        for (Resource const resource : createInfo.writes)
        {
            check(_textures.find(resource) != _textures.end());
            imageViews.push_back(_textures.at(resource)[i]->GetImageView());
        }

        vk::FramebufferCreateInfo framebufferInfo{};
        framebufferInfo.renderPass = _renderPasses.at(pass);
        framebufferInfo.attachmentCount = imageViews.size();
        framebufferInfo.pAttachments = imageViews.data();
        framebufferInfo.width = outResolution.width;
        framebufferInfo.height = outResolution.height;
        framebufferInfo.layers = 1;

        vk::Framebuffer framebuffer;
        device->CreateFramebuffer(framebufferInfo, &framebuffer, createInfo.debugName + "<framebuffer>");
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

    device->CreateDescriptorPool(descriptorPoolInfo, &_uboDescriptorPool, "<graph_ubo_descriptor_pool>");

    std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings{
        {0, vk::DescriptorType::eUniformBuffer, 1, {vk::ShaderStageFlagBits::eAllGraphics}}};

    vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
    descriptorSetLayoutInfo.bindingCount = static_cast<uint32_t>(descriptorSetLayoutBindings.size());
    descriptorSetLayoutInfo.pBindings = descriptorSetLayoutBindings.data();

    device->CreateDescriptorSetLayout(descriptorSetLayoutInfo, &_uboDescriptorSetLayout,
                                      "<graph_ubo_descriptor_set_layout>");

    std::vector<vk::WriteDescriptorSet> writeDescriptors{};
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vk::Buffer uboBuffer;
        vk::DeviceMemory uboDeviceMemory;
        void *uboMappedMemories = nullptr;

        vk::BufferCreateInfo createInfo;
        createInfo.size = _uboSize;
        createInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer;

        device->CreateBufferAndBindMemory(createInfo, &uboBuffer, &uboDeviceMemory,
                                          {vk::MemoryPropertyFlagBits::eHostVisible},
                                          fmt::format("<ubo_buffer>({})", i));

        _uboBuffers[i] = uboBuffer;
        _uboDeviceMemories[i] = uboDeviceMemory;

        device->MapMemory(uboDeviceMemory, &uboMappedMemories);

        _uboMappedMemories[i] = uboMappedMemories;

        vk::DescriptorSet descriptorSet;

        vk::DescriptorSetAllocateInfo setAllocInfo{};
        setAllocInfo.descriptorPool = _uboDescriptorPool;
        setAllocInfo.descriptorSetCount = 1u;
        setAllocInfo.pSetLayouts = &_uboDescriptorSetLayout;

        device->AllocateDescriptorSet(setAllocInfo, &descriptorSet, "<ubo_descriptor_set>" + std::to_string(i));

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

void Graph::BuildDescriptors(Resource resource)
{
    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    ResourceCreateInfo const &resourceInfo = GetResourceInfo(resource);

    check(IsSampled(resource));
    check(_validResources.find(resource) != _validResources.end());

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vk::DescriptorSet descriptorSet;

        vk::DescriptorSetAllocateInfo setAllocInfo{};
        setAllocInfo.descriptorPool = _descriptorPool;
        setAllocInfo.descriptorSetCount = 1u;
        setAllocInfo.pSetLayouts = &_descriptorSetLayout;

        device->AllocateDescriptorSet(setAllocInfo, &descriptorSet, resourceInfo.debugName + "<descriptor_set>");

        check(_textures.find(resource) != _textures.end());

        vk::DescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        imageInfo.imageView = _textures.at(resource)[i]->GetImageView();

        if (resourceInfo.usage == ResourceUsage::eDepth)
        {
            imageInfo.sampler = _depthSampler->GetSampler();
        }
        else
        {
            imageInfo.sampler = _colorSampler->GetSampler();
        }

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

void Graph::Resize(uint32_t width, uint32_t height)
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

void Graph::OnResizeCallback(void *graph, uint32_t width, uint32_t height)
{
    check(graph);
    reinterpret_cast<Graph *>(graph)->Resize(width, height);
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
