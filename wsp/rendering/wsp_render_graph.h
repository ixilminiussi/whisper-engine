#ifndef WSP_RENDER_GRAPH
#define WSP_RENDER_GRAPH

#include <functional>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>

namespace wsp
{

struct ResourceHandle
{
    size_t id;
};

struct RGLifetimeDescription
{
    size_t firstUse{std::numeric_limits<size_t>::max()};
    size_t lastUse{0};

    bool isWritten{false};
    bool isSampled{false};

    std::vector<size_t> users{};
};

struct RGResourceDescription
{
    std::string format;
    size_t width, height;

    friend class RenderGraph;

  private:
    RGLifetimeDescription lifetimeDescription{};
};

struct GPUImage
{
    std::string debugName;
};

class RGContext
{
  public:
    RGContext(std::unordered_map<size_t, GPUImage *> &gpuResources) : _gpuResources(gpuResources) {};

    GPUImage *GetImageView(ResourceHandle handle);

  private:
    std::unordered_map<size_t, GPUImage *> &_gpuResources;
};

struct RenderPass
{
    std::string name;
    std::vector<ResourceHandle> reads;
    std::vector<ResourceHandle> writes;
    std::function<void(RGContext &)> execute;
};

class RGBuilder
{
  public:
    RGBuilder(std::unordered_map<std::string, ResourceHandle> &nameToHandle, std::vector<RenderPass> &passList,
              std::vector<RGResourceDescription> &resourceDescs, std::vector<GPUImage *> &dummyGpuImages,
              std::vector<ResourceHandle> &reads, std::vector<ResourceHandle> &writes)
        : _nameToHandle(nameToHandle), _passes(passList), _descriptions(resourceDescs), _images(dummyGpuImages),
          _readHandles(reads), _writeHandles(writes) {};

    ResourceHandle Write(const std::string &name);

    ResourceHandle Read(const std::string &name);

    void SetExecute(std::function<void(RGContext &)> func);

    std::function<void(RGContext &)> ConsumeExecute();

  private:
    std::unordered_map<std::string, ResourceHandle> &_nameToHandle;
    std::vector<RenderPass> &_passes;
    std::vector<RGResourceDescription> &_descriptions;
    std::vector<GPUImage *> &_images;
    std::vector<ResourceHandle> &_readHandles;
    std::vector<ResourceHandle> &_writeHandles;
    std::function<void(RGContext &)> _pendingExecute{nullptr};

    ResourceHandle GetOrCreate(const std::string &name, bool is_write);
};

class RenderGraph
{
  public:
    void DeclareResource(const std::string &name, const RGResourceDescription &desc);

    void AddPass(const std::string &name, std::function<void(RGBuilder &)> buildFunction);

    void Compile();
    void Execute();

  private:
    std::unordered_map<std::string, ResourceHandle> _nameToHandle;
    std::vector<RGResourceDescription> _resourceDescriptions;
    std::vector<GPUImage *> _gpuImages;
    std::vector<RenderPass> _passes;

    std::unordered_map<size_t, GPUImage *> GPUImagesMap();
};

} // namespace wsp

#endif
