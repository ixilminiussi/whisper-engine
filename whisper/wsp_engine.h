#ifndef WSP_ENGINE
#define WSP_ENGINE

namespace wsp
{

class Device;

namespace engine
{

bool Initialize();
void Run();
void Terminate();

Device *GetDevice();

} // namespace engine

} // namespace wsp

#endif
