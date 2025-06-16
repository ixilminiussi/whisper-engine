#include "wsp_render_graph.h"

// lib
#include <spdlog/spdlog.h>

// std
#include <stdexcept>

namespace wsp
{

GPUImage *RGContext::GetImageView(ResourceHandle handle)
{
    if (_gpuResources.find(handle.id) == _gpuResources.end())
    {
        throw std::out_of_range("GPU Resource not found");
    }
    return _gpuResources[handle.id];
}

ResourceHandle RGBuilder::Write(const std::string &name)
{
    return GetOrCreate(name, /*is_write=*/true);
}

ResourceHandle RGBuilder::Read(const std::string &name)
{
    return GetOrCreate(name, /*is_write=*/false);
}

void RGBuilder::SetExecute(std::function<void(RGContext &)> func)
{
    _pendingExecute = func;
}

std::function<void(RGContext &)> RGBuilder::ConsumeExecute()
{
    if (_pendingExecute == nullptr)
    {
        throw std::runtime_error("Forgot to assign execute function to render pass");
    }
    return _pendingExecute;
}

ResourceHandle RGBuilder::GetOrCreate(const std::string &name, bool is_write)
{
    if (_nameToHandle.find(name) == _nameToHandle.end())
    {
        size_t id = static_cast<size_t>(_descriptions.size());
        _nameToHandle[name] = {id};
        RGResourceDescription rd{};
        rd.format = "RGBA8";
        rd.width = 1280;
        rd.height = 1280;
        _descriptions.push_back(rd);
        _images.push_back(new GPUImage{name});
    }
    ResourceHandle handle = _nameToHandle[name];
    if (is_write)
        _writeHandles.push_back(handle);
    else
        _readHandles.push_back(handle);
    return handle;
}

void RenderGraph::DeclareResource(const std::string &name, const RGResourceDescription &desc)
{
    if (_nameToHandle.find(name) == _nameToHandle.end())
    {
        size_t id = static_cast<size_t>(_resourceDescriptions.size());
        _nameToHandle[name] = {id};
        _resourceDescriptions.push_back(desc);
        _gpuImages.push_back(new GPUImage{name});
    }
}

void RenderGraph::AddPass(const std::string &name, std::function<void(RGBuilder &)> buildFunction)
{
    std::vector<ResourceHandle> reads, writes;
    RGBuilder builder(_nameToHandle, _passes, _resourceDescriptions, _gpuImages, reads, writes);
    buildFunction(builder);

    RenderPass pass;
    pass.name = name;
    pass.reads = reads;
    pass.writes = writes;
    pass.execute = builder.ConsumeExecute();

    _passes.push_back(pass);
}

void RenderGraph::Compile()
{
    spdlog::info("Compiling render passes:");
    size_t i = 0;
    for (auto &pass : _passes)
    {
        for (ResourceHandle &handle : pass.reads)
        {
            RGLifetimeDescription &lifetimeDescription = _resourceDescriptions[handle.id].lifetimeDescription;
            lifetimeDescription.firstUse = std::min(lifetimeDescription.firstUse, i);
            lifetimeDescription.lastUse = std::max(lifetimeDescription.lastUse, i);
            lifetimeDescription.isSampled = true;
            lifetimeDescription.users.push_back(i);
        }

        for (ResourceHandle &handle : pass.writes)
        {
            RGLifetimeDescription &lifetimeDescription = _resourceDescriptions[handle.id].lifetimeDescription;
            lifetimeDescription.firstUse = std::min(lifetimeDescription.firstUse, i);
            lifetimeDescription.lastUse = std::max(lifetimeDescription.lastUse, i);
            lifetimeDescription.isWritten = true;
            lifetimeDescription.users.push_back(i);
        }
        i++;
    }
}

void RenderGraph::Execute()
{
    std::unordered_map<size_t, GPUImage *> gpuImages = GPUImagesMap();
    RGContext context(gpuImages);

    spdlog::info("Executing render passes:");
    for (auto &pass : _passes)
    {
        spdlog::info(" → {}", pass.name);
        pass.execute(context);
    }
}

std::unordered_map<size_t, GPUImage *> RenderGraph::GPUImagesMap()
{
    std::unordered_map<size_t, GPUImage *> result;
    for (size_t i = 0; i < _gpuImages.size(); ++i)
    {
        result[i] = _gpuImages[i];
    }
    return result;
}

} // namespace wsp
