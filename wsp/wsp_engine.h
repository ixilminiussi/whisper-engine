#ifndef WSP_ENGINE_H
#define WSP_ENGINE_H

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
