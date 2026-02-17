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

#include <spdlog/fmt/bundled/base.h>
#include <spdlog/spdlog.h>

#include <vulkan/vulkan.hpp>

#include <optional>
#include <stdexcept>
#ifdef _WIN32
#include <direct.h>
#include <io.h>
#include <windows.h>
#else
#include <unistd.h>
#endif
#include <variant>

using namespace wsp;

uint32_t const Graph::SAMPLER_DEPTH{0};
uint32_t const Graph::SAMPLER_COLOR_CLAMPED{1};
uint32_t const Graph::SAMPLER_COLOR_REPEATED{2};

Graph::Graph(uint32_t width, uint32_t height)
    : _passInfos{}, _resourceInfos{}, _passes{}, _resources{}, _target{0}, _width{width}, _height{height}, _uboSize{0},
      _uboDescriptorSets{}, _uboBuffers{}, _uboDeviceMemories{}, _currentFrameIndex{0}
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

    device->WaitIdle();

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
    _passes.push_back({});
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

    for (Pass const writer : _resources[resource.index].writers)
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

    for (Resource const &write : _passInfos[pass.index].writes)
    {
        FindDependencies(validResources, validPasses, write, visitingStack);
    }

    for (Resource const &read : _passInfos[pass.index].reads)
    {
        FindDependencies(validResources, validPasses, read, visitingStack);
    }

    visitingStack.erase(pass);
}

void Graph::Compile(Resource target, GraphUsage usage)
{
    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    device->WaitIdle();

    _target = target;
    _usage = usage;

    Reset();

    for (uint32_t pass = 0; pass < _passInfos.size(); pass++)
    {
        PassCreateInfo const &passInfo = _passInfos[pass];

        if (!(passInfo.writes.empty() ||
              std::all_of(passInfo.writes.begin() + 1, passInfo.writes.end(), [&](Resource resource) {
                  return _resourceInfos[resource.index].extent == _resourceInfos[passInfo.writes[0].index].extent;
              })))
        {
            std::ostringstream oss;
            oss << "Graph: resolution mismatch on pass " << pass;
            throw std::runtime_error(oss.str());
        }

        for (Resource const resource : passInfo.reads)
        {
            check(_resourceInfos.size() > resource.index);
            _resources[resource.index].readers.emplace_back(pass);
        }
        for (Resource const resource : passInfo.writes)
        {
            check(_resourceInfos.size() > resource.index);
            _resources[resource.index].writers.emplace_back(pass);
            _passes[pass].rebuildOnChange = true;
        }
        for (Pass const dependency : passInfo.passDependencies)
        {
            _passes[dependency.index].dependencies.push_back(Pass{pass});
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
        oss << _resourceInfos[resource.index].debugName << ", ";
    }
    for (Pass const pass : _validPasses)
    {
        oss << _passInfos[pass.index].debugName << ", ";
    }
    spdlog::info("{0}", oss.str());

    KhanFindOrder(_validResources, _validPasses);

    _requestsUniform = false;

    for (Pass const pass : _validPasses)
    {
        if (_passInfos[pass.index].readsUniform)
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

    return _resources[_target.index].images[_currentFrameIndex];
}

vk::DescriptorSet Graph::GetTargetDescriptorSet() const
{
    if (_usage != GraphUsage::eToDescriptorSet)
    {
        throw std::runtime_error("Graph: usage must be eToDescriptorSet in order to get target descriptor set");
    }

    return _resources[_target.index].descriptorSets[_currentFrameIndex];
}

void Graph::ChangeUsage(GraphUsage usage)
{
    Compile(_target, usage);
}

void Graph::BuildPipeline(Pass pass)
{
    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    PassCreateInfo const &passInfo = _passInfos[pass.index];

    std::vector<char> const vertCode = ReadShaderFile(passInfo.vertFile);
    std::vector<char> const fragCode = ReadShaderFile(passInfo.fragFile);

    PipelineHolder &pipelineHolder = _passes[pass.index].pipeline;
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
    rasterizationInfo.depthBiasEnable = vk::True;
    rasterizationInfo.depthBiasConstantFactor = 1.0f;
    rasterizationInfo.depthBiasClamp = 0.0f;
    rasterizationInfo.depthBiasSlopeFactor = 2.0f;

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
        ResourceCreateInfo const &resourceInfo = _resourceInfos[resource.index];
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
    pipelineInfo.renderPass = _passes[pass.index].renderPass;
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
    ZoneScopedN("graph render");

    _currentFrameIndex = frameIndex;

    extern TracyVkCtx TRACY_CTX;
    TracyVkZone(TRACY_CTX, commandBuffer, "graph");

    if (_populateUbo)
    {
        ZoneScopedN("populate ubo");
        void *ubo = _populateUbo();
        FlushUbo(ubo);
    }

    for (Pass const pass : _orderedPasses)
    {
        ZoneScopedN("run passes");

        PassHolder const &passHolder = _passes[pass.index];

        int i = 0;
        for (std::array<vk::ImageMemoryBarrier, MAX_FRAMES_IN_FLIGHT> const &barriers : passHolder.imageMemoryBarriers)
        {
            vk::ImageMemoryBarrier const &barrier = barriers[_currentFrameIndex];

            commandBuffer.pipelineBarrier(
                vk::PipelineStageFlagBits::eLateFragmentTests | vk::PipelineStageFlagBits::eColorAttachmentOutput,
                vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests |
                    vk::PipelineStageFlagBits::eFragmentShader,
                vk::DependencyFlagBits::eByRegion, 0, nullptr, 0, nullptr, 1, &barrier);

            i++;
        }

        commandBuffer.beginRenderPass(passHolder.renderPassBeginInfo[_currentFrameIndex], vk::SubpassContents::eInline);

        PassCreateInfo const &passInfo = _passInfos[pass.index];

        extern TracyVkCtx TRACY_CTX;
        TracyVkZoneTransient(TRACY_CTX, __tracy, commandBuffer, passInfo.debugName.c_str(), true);

        commandBuffer.setViewport(0, 1, &passHolder.viewport);
        commandBuffer.setScissor(0, 1, &passHolder.scissor);

        uint32_t offset = 0;

        if (passInfo.readsUniform)
        {
            commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, passHolder.pipeline.pipelineLayout,
                                             offset, 1u, &_uboDescriptorSets[_currentFrameIndex], 0u, nullptr);
            offset++;
        }
        if (passInfo.reads.size() > 0)
        {
            std::vector<vk::DescriptorSet> descriptorSets;
            descriptorSets.reserve(passInfo.reads.size());

            for (Resource const resource : passInfo.reads)
            {
                descriptorSets.push_back(_resources[resource.index].descriptorSets[_currentFrameIndex]);
            }

            commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, passHolder.pipeline.pipelineLayout,
                                             offset, descriptorSets.size(), descriptorSets.data(), 0u, nullptr);

            offset += passInfo.reads.size();
        }

        for (StaticTextures const *staticTextures : passInfo.staticTextures)
        {
            vk::DescriptorSet const staticDescriptorSet = staticTextures->GetDescriptorSet();
            commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, passHolder.pipeline.pipelineLayout,
                                             offset, 1u, &staticDescriptorSet, 0u, nullptr);
            offset++;
        }

        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, passHolder.pipeline.pipeline);
        passInfo.execute(commandBuffer, passHolder.pipeline.pipelineLayout);
        commandBuffer.endRenderPass();
    }
}

void Graph::Reset()
{
    ZoneScopedN("resetting graph");
    spdlog::debug("Graph: resetting began...");

    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    if (_uboDescriptorPool != VK_NULL_HANDLE)
    {
        FreeUbo();
    }

    for (ResourceHolder &resource : _resources)
    {
        resource.writers.clear();
        resource.readers.clear();
    }
    for (PassHolder &pass : _passes)
    {
        pass.dependencies.clear();
    }

    for (Resource const resource : _validResources)
    {
        Free(resource);
    }
    for (Pass const pass : _validPasses)
    {
        Free(pass);
    }

    _passes.clear();
    _passes.resize(_passInfos.size(), {});
    _resources.clear();
    _resources.resize(_resourceInfos.size(), {});

    spdlog::debug("Graph: resetting complete...");
}

void Graph::Free(Resource resource)
{
    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    check(_validResources.find(resource) != _validResources.end());

    ResourceHolder &resourceHolder = _resources[resource.index];

    for (vk::DescriptorSet &descriptorSet : resourceHolder.descriptorSets)
    {
        device->FreeDescriptorSet(_descriptorPool, &descriptorSet);
    }
    resourceHolder.descriptorSets = {};

    for (Image *image : resourceHolder.images)
    {
        delete image;
    }
    resourceHolder.images = {};

    for (Texture *texture : resourceHolder.textures)
    {
        delete texture;
    }
    resourceHolder.textures = {};

    spdlog::debug("Graph: freed resource '{}'", _resourceInfos[resource.index].debugName);
}

void Graph::Free(Pass pass)
{
    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    check(_validPasses.find(pass) != _validPasses.end());

    PassHolder &passHolder = _passes[pass.index];

    PipelineHolder &pipelineHolder = passHolder.pipeline;
    device->DestroyShaderModule(&pipelineHolder.vertShaderModule);
    device->DestroyShaderModule(&pipelineHolder.fragShaderModule);
    device->DestroyPipelineLayout(&pipelineHolder.pipelineLayout);
    device->DestroyGraphicsPipeline(&pipelineHolder.pipeline);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vk::Framebuffer &framebuffer = passHolder.frameBuffers[i];
        device->DestroyFramebuffer(&framebuffer);
    }

    passHolder.imageMemoryBarriers.clear();

    vk::RenderPass &renderPass = passHolder.renderPass;
    device->DestroyRenderPass(&renderPass);

    spdlog::debug("Graph: freed pass '{}'", _passInfos[pass.index].debugName);
}

void Graph::KhanFindOrder(std::set<Resource> const &resources, std::set<Pass> const &passes)
{
    _orderedPasses.clear();

    std::map<Resource, int> resourceDegrees{};
    std::map<Pass, int> passDegrees{};

    for (Resource const resource : resources)
    {
        resourceDegrees[resource] = _resources[resource.index].writers.size();
    }

    for (Pass const pass : passes)
    {
        PassCreateInfo const &passInfo = _passInfos[pass.index];
        passDegrees[pass] = passInfo.reads.size() + passInfo.passDependencies.size();
    }

    while (passDegrees.size() > 0 && resourceDegrees.size() > 0)
    {
        bool progress = false;

        for (auto it = resourceDegrees.begin(); it != resourceDegrees.end();)
        {
            if (it->second == 0)
            {
                ResourceHolder const &resourceHolder = _resources[it->first.index];
                for (Pass const reader : resourceHolder.readers)
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
                PassCreateInfo const &passInfo = _passInfos[it->first.index];
                for (Resource const writey : passInfo.writes)
                {
                    resourceDegrees.at(writey)--;
                }
                PassHolder const &passHolder = _passes[it->first.index];
                for (Pass const dependant : passHolder.dependencies)
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
                ResourceCreateInfo const &resourceInfo = _resourceInfos[resource.index];
                oss << resourceInfo.debugName << ": " << degree << ", ";
            }
            for (auto const &[pass, degree] : passDegrees)
            {
                PassCreateInfo const &passInfo = _passInfos[pass.index];
                oss << passInfo.debugName << ": " << degree << ", ";
            }
            throw std::runtime_error(oss.str());
        }
    }

    std::ostringstream oss;
    oss << "Graph: pass order selected: ";
    for (int i = 0; i < _orderedPasses.size(); i++)
    {
        PassCreateInfo const &passInfo = _passInfos[_orderedPasses[i].index];

        if (i != 0)
        {
            oss << " -> ";
        }
        oss << passInfo.debugName;
    }
    spdlog::info("{0}", oss.str());
}

void Graph::Build(Resource resource)
{
    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    ResourceCreateInfo const &createInfo = _resourceInfos[resource.index];

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

    ResourceHolder const &resourceHolder = _resources[resource.index];
    if (resourceHolder.writers.size() > 0)
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
        ResourceHolder &resourceHolder = _resources[resource.index];

        resourceHolder.images[i] = new Image(device, imageInfo, createInfo.debugName + std::string("_image"));

        Texture::CreateInfo textureCreateInfo{};
        textureCreateInfo.pImage = resourceHolder.images[i];
        textureCreateInfo.name = createInfo.debugName;
        textureCreateInfo.pSampler = _colorSampler;

        if (createInfo.usage == ResourceUsage::eDepth)
        {
            textureCreateInfo.depth = true;
            textureCreateInfo.pSampler = _depthSampler;
        }

        resourceHolder.textures[i] = new Texture(device, textureCreateInfo);
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

    PassCreateInfo const &createInfo = _passInfos[pass.index];
    PassHolder &passHolder = _passes[pass.index];

    vk::Extent2D outResolution{(uint32_t)_width, (uint32_t)_height};

    std::vector<vk::AttachmentReference> colorAttachmentReferences{};
    std::optional<vk::AttachmentReference> depthAttachmentReference{};
    colorAttachmentReferences.reserve(createInfo.writes.size());

    std::vector<vk::AttachmentDescription> attachmentDescriptions{};
    attachmentDescriptions.reserve(createInfo.writes.size());

    int i = 0;
    for (Resource const resource : createInfo.writes)
    {
        ResourceCreateInfo const &resourceInfo = _resourceInfos[resource.index];

        if (resourceInfo.extent != vk::Extent2D{0, 0})
        {
            outResolution = resourceInfo.extent;
        }

        ResourceHolder const &resourceHolder = _resources[resource.index];

        bool firstWriter = false;
        bool nextIsRead = false;

        if (resourceHolder.writers[0] == pass)
        {
            firstWriter = true;
        }
        if (resourceHolder.writers[resourceHolder.writers.size() - 1] == pass && IsSampled(resource))
        {
            nextIsRead = true;
        }

        std::array<vk::ImageMemoryBarrier, MAX_FRAMES_IN_FLIGHT> imageMemoryBarriers;
        vk::ImageMemoryBarrier imageMemoryBarrier{};

        imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
        imageMemoryBarrier.subresourceRange.levelCount = 1;
        imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
        imageMemoryBarrier.subresourceRange.layerCount = 1;
        imageMemoryBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        imageMemoryBarrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
        imageMemoryBarrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
        imageMemoryBarrier.oldLayout = vk::ImageLayout::eColorAttachmentOptimal;
        imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
        imageMemoryBarrier.newLayout = vk::ImageLayout::eColorAttachmentOptimal;
        imageMemoryBarrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

        if (resourceInfo.usage == ResourceUsage::eDepth)
        {
            imageMemoryBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
            imageMemoryBarrier.oldLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
            imageMemoryBarrier.newLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
            imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
            imageMemoryBarrier.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
        }

        if (!firstWriter && !nextIsRead)
        {
            for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
            {
                imageMemoryBarrier.image = _resources[resource.index].images[i]->GetImage();

                imageMemoryBarriers[i] = imageMemoryBarrier;
            }

            passHolder.imageMemoryBarriers.push_back(imageMemoryBarriers);
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
            imageMemoryBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
            if (!firstWriter)
            {
                attachment.loadOp = vk::AttachmentLoadOp::eLoad;
                attachment.initialLayout = vk::ImageLayout::eColorAttachmentOptimal;
            }
            attachment.storeOp = vk::AttachmentStoreOp::eStore;
            if (nextIsRead)
            {
                attachment.finalLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
            }
            else
            {
                attachment.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;
            }

            attachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

            colorAttachmentReferences.push_back(attachmentRef);
            break;
        case ResourceUsage::eDepth:
            imageMemoryBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
            if (!firstWriter)
            {
                attachment.loadOp = vk::AttachmentLoadOp::eLoad;
                attachment.initialLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
            }
            attachment.storeOp = vk::AttachmentStoreOp::eDontCare;
            if (nextIsRead)
            {
                attachment.finalLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
            }
            else
            {
                attachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
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

    vk::RenderPassCreateInfo renderPassCreateInfo{};
    renderPassCreateInfo.attachmentCount = attachmentDescriptions.size();
    renderPassCreateInfo.pAttachments = attachmentDescriptions.data();
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpass;

    vk::RenderPass renderPass;
    device->CreateRenderPass(renderPassCreateInfo, &renderPass, createInfo.debugName + "<render_pass>");
    passHolder.renderPass = renderPass;

    std::vector<vk::ClearValue> &clearValues = passHolder.clearValues;
    clearValues.clear();
    clearValues.reserve(createInfo.writes.size());

    for (Resource const resource : createInfo.writes)
    {
        clearValues.push_back(_resourceInfos[resource.index].clear);
    }

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        std::vector<vk::ImageView> imageViews{};
        for (Resource const resource : createInfo.writes)
        {
            imageViews.push_back(_resources[resource.index].textures[i]->GetImageView());
        }

        vk::FramebufferCreateInfo framebufferInfo{};
        framebufferInfo.renderPass = passHolder.renderPass;
        framebufferInfo.attachmentCount = imageViews.size();
        framebufferInfo.pAttachments = imageViews.data();
        framebufferInfo.width = outResolution.width;
        framebufferInfo.height = outResolution.height;
        framebufferInfo.layers = 1;

        vk::Framebuffer framebuffer;
        device->CreateFramebuffer(framebufferInfo, &framebuffer, createInfo.debugName + "<framebuffer>");
        passHolder.frameBuffers[i] = framebuffer;

        vk::RenderPassBeginInfo renderPassBeginInfo{};
        renderPassBeginInfo.renderPass = passHolder.renderPass;
        renderPassBeginInfo.framebuffer = framebuffer;

        renderPassBeginInfo.renderArea.offset = vk::Offset2D{0, 0};

        if (ResourceCreateInfo const &resourceInfo = _resourceInfos[createInfo.writes.at(0).index];
            resourceInfo.extent != vk::Extent2D{0u, 0u})
        {
            renderPassBeginInfo.renderArea.extent = resourceInfo.extent;
        }
        else
        {
            renderPassBeginInfo.renderArea.extent = vk::Extent2D{uint32_t(_width), (uint32_t)_height};
        }

        renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassBeginInfo.pClearValues = clearValues.data();

        passHolder.renderPassBeginInfo[i] = renderPassBeginInfo;
    }

    vk::Viewport viewport{};
    viewport.x = 0.f;
    viewport.y = 0.f;
    viewport.minDepth = 0.f;
    viewport.maxDepth = 1.f;

    vk::Rect2D scissor{};
    scissor.offset = vk::Offset2D{0, 0};

    if (ResourceCreateInfo const &resourceInfo = _resourceInfos[createInfo.writes.at(0).index];
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

    passHolder.viewport = viewport;
    passHolder.scissor = scissor;

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

    ResourceCreateInfo const &resourceInfo = _resourceInfos[resource.index];

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

        ResourceHolder &resourceHolder = _resources[resource.index];

        vk::DescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        imageInfo.imageView = resourceHolder.textures[i]->GetImageView();

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

        resourceHolder.descriptorSets[i] = descriptorSet;
    }

    spdlog::debug("Graph: built descriptors for '{}'", resourceInfo.debugName);
}

void Graph::Resize(uint32_t width, uint32_t height)
{
    spdlog::debug("Graph: resizing from {}x{} to {}x{}", _width, _height, width, height);

    Device const *device = SafeDeviceAccessor::Get();
    check(device);

    device->WaitIdle();

    _width = width;
    _height = height;

    std::vector<Resource> resourcesToRebuild;
    resourcesToRebuild.reserve(_validResources.size());
    std::vector<Pass> passesToRebuild;
    passesToRebuild.reserve(_validPasses.size());

    for (Resource const resource : _validResources)
    {
        ResourceCreateInfo const &resourceInfo = _resourceInfos[resource.index];

        if (resourceInfo.extent == vk::Extent2D{0, 0})
        {
            Free(resource);
            resourcesToRebuild.push_back(resource);
        }
    }
    for (Pass const pass : _validPasses)
    {
        PassHolder const &passHolder = _passes[pass.index];

        if (passHolder.rebuildOnChange)
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
        return _resources[resource.index].readers.size() > 0 && resource != _target;
    }
    return _resources[resource.index].readers.size() > 0 || resource == _target;
}
