#ifndef WSP_ENGINE
#define WSP_ENGINE

#include <vulkan/vulkan.hpp>
//
#include <tracy/TracyVulkan.hpp>

namespace wsp
{

class Device;

namespace engine
{

bool Initialize();
void Run();
void Terminate();

TracyVkCtx TracyCtx();

} // namespace engine

} // namespace wsp

#endif
