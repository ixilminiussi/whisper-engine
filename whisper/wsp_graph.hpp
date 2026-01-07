#ifndef WSP_GRAPH
#define WSP_GRAPH

#include <wsp_constants.hpp>
#include <wsp_handles.hpp>

#include <vulkan/vulkan.hpp>

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

    struct ResourceHolder
    {
        std::array<class Image *, MAX_FRAMES_IN_FLIGHT> images;
        std::array<class Texture *, MAX_FRAMES_IN_FLIGHT> textures;
        std::array<vk::DescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptorSets;

        std::vector<Pass> writers{};
        std::vector<Pass> readers{};
    };

    struct PassHolder
    {
        std::array<vk::Framebuffer, MAX_FRAMES_IN_FLIGHT> frameBuffers;
        vk::RenderPass renderPass;
        std::array<vk::RenderPassBeginInfo, MAX_FRAMES_IN_FLIGHT> renderPassBeginInfo;
        vk::Viewport viewport;
        vk::Rect2D scissor;
        PipelineHolder pipeline; // in future, maybe point to an array of pipelines..? Or focus on the ONE
                                 // GIGA SHADER Unity approach?
        std::vector<vk::ClearValue> clearValues;

        bool rebuildOnChange{false};
        std::vector<Pass> dependencies{};
        std::vector<std::array<vk::ImageMemoryBarrier, MAX_FRAMES_IN_FLIGHT>> imageMemoryBarriers{};
    };

    bool IsSampled(Resource);

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

    void FindDependencies(std::set<Resource> *validResources, std::set<Pass> *validPasses, Resource pass,
                          std::set<std::variant<Resource, Pass>> &visitingStack);
    void FindDependencies(std::set<Resource> *validResources, std::set<Pass> *validPasses, Pass pass,
                          std::set<std::variant<Resource, Pass>> &visitingStack);

    void Reset();

    class Sampler *_depthSampler;
    class Sampler *_colorSampler;

    std::function<void *()> _populateUbo;

    std::vector<PassCreateInfo> _passInfos;
    std::vector<ResourceCreateInfo> _resourceInfos;
    Resource _target;
    GraphUsage _usage;

    std::vector<PassHolder> _passes;
    std::vector<ResourceHolder> _resources;

    std::set<Resource> _validResources;
    std::set<Pass> _validPasses;
    std::vector<Pass> _orderedPasses;

    vk::DescriptorPool _descriptorPool;
    vk::DescriptorSetLayout _descriptorSetLayout;

    bool _requestsUniform;
    uint32_t _uboSize;
    std::array<vk::Buffer, MAX_FRAMES_IN_FLIGHT> _uboBuffers;
    std::array<vk::DeviceMemory, MAX_FRAMES_IN_FLIGHT> _uboDeviceMemories;
    std::array<vk::DescriptorSet, MAX_FRAMES_IN_FLIGHT> _uboDescriptorSets;
    std::array<void *, MAX_FRAMES_IN_FLIGHT> _uboMappedMemories;
    vk::DescriptorPool _uboDescriptorPool;
    vk::DescriptorSetLayout _uboDescriptorSetLayout;

    uint32_t _width, _height;
    uint32_t _currentFrameIndex; // only accurate when calling FlushUbo to update
};

} // namespace wsp

#endif
