#ifndef WSP_GRAPH
#define WSP_GRAPH

#include <wsp_constants.hpp>
#include <wsp_handles.hpp>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_handles.hpp>

#include <map>
#include <set>
#include <variant>

namespace wsp
{

class Graph
{
  public:
    enum GraphUsage
    {
        eToTransfer = 0,
        eToDescriptorSet = 1
    };

    static uint32_t const SAMPLER_DEPTH;
    static uint32_t const SAMPLER_COLOR_CLAMPED;
    static uint32_t const SAMPLER_COLOR_REPEATED;
    static uint32_t const MAX_SAMPLER_SETS;

    Graph(uint32_t width, uint32_t height);
    ~Graph();

    Graph(Graph const &) = delete;
    Graph &operator=(Graph const &) = delete;

    void SetUboSize(uint32_t uboSize);
    void SetPopulateUboFunction(std::function<void *()>);
    [[nodiscard]] Resource NewResource(const struct ResourceCreateInfo &);
    Pass NewPass(const struct PassCreateInfo &);

    void Compile(Resource target, GraphUsage);
    void Render(vk::CommandBuffer, uint32_t frameIndex);

    class Image *GetTargetImage() const;
    vk::DescriptorSet GetTargetDescriptorSet() const;

    void ChangeUsage(GraphUsage);
    void Resize(uint32_t width, uint32_t height);
    static void OnResizeCallback(void *, uint32_t width, uint32_t height);

  protected:
    void FlushUbo(void *ubo);

    struct PipelineHolder
    {
        vk::ShaderModule vertShaderModule;
        vk::ShaderModule fragShaderModule;
        vk::PipelineLayout pipelineLayout;
        vk::Pipeline pipeline;
    };

    bool IsSampled(Resource resouce);

    void KhanFindOrder(std::set<Resource> const &, std::set<Pass> const &);

    void Build(Resource);
    void Build(Pass);
    void BuildUbo();
    void FreeUbo();

    void Free(Resource);
    void Free(Pass);

    void BuildPipeline(Pass);
    void BuildSamplers(class Device const *);
    void BuildDescriptorPool();
    void BuildDescriptors(Resource);

    bool FindDependencies(std::set<Resource> *validResources, std::set<Pass> *validPasses, Resource pass,
                          std::set<std::variant<Resource, Pass>> &visitingStack);
    bool FindDependencies(std::set<Resource> *validResources, std::set<Pass> *validPasses, Pass pass,
                          std::set<std::variant<Resource, Pass>> &visitingStack);

    void Reset();

    class Sampler *_depthSampler;
    class Sampler *_colorSampler;

    std::function<void *()> _populateUbo;

    PassCreateInfo const &GetPassInfo(Pass) const;
    ResourceCreateInfo const &GetResourceInfo(Resource) const;
    vk::RenderPass GetRenderPass(Pass) const;

    std::vector<PassCreateInfo> _passInfos;
    std::vector<ResourceCreateInfo> _resourceInfos;
    Resource _target;
    GraphUsage _usage;

    std::map<Resource, std::array<class Image *, MAX_FRAMES_IN_FLIGHT>> _images;
    std::map<Resource, std::array<class Texture *, MAX_FRAMES_IN_FLIGHT>> _textures;

    std::map<Pass, std::array<vk::Framebuffer, MAX_FRAMES_IN_FLIGHT>> _framebuffers;
    std::map<Pass, std::vector<std::array<vk::ImageMemoryBarrier, MAX_FRAMES_IN_FLIGHT>>> _imageMemoryBarriers;
    std::map<Pass, vk::RenderPass> _renderPasses;
    std::map<Pass, PipelineHolder> _pipelines; // in future, maybe point to an array of pipelines..? Or focus on the ONE
                                               // GIGA SHADER Unity approach?

    vk::DescriptorPool _descriptorPool;
    vk::DescriptorSetLayout _descriptorSetLayout;
    std::map<Resource, std::array<vk::DescriptorSet, MAX_FRAMES_IN_FLIGHT>> _descriptorSets;

    bool _requestsUniform;
    uint32_t _uboSize;
    std::array<vk::Buffer, MAX_FRAMES_IN_FLIGHT> _uboBuffers;
    std::array<vk::DeviceMemory, MAX_FRAMES_IN_FLIGHT> _uboDeviceMemories;
    std::array<vk::DescriptorSet, MAX_FRAMES_IN_FLIGHT> _uboDescriptorSets;
    std::array<void *, MAX_FRAMES_IN_FLIGHT> _uboMappedMemories;
    vk::DescriptorPool _uboDescriptorPool;
    vk::DescriptorSetLayout _uboDescriptorSetLayout;

    std::vector<Pass> _orderedPasses;
    std::set<Resource> _validResources;
    std::set<Pass> _validPasses;

    uint32_t _width, _height;
    uint32_t _currentFrameIndex; // only accurate when calling FlushUbo to update
};

} // namespace wsp

#endif
