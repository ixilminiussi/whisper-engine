#ifndef WSP_ENGINE
#define WSP_ENGINE

#include <vector>

namespace wsp
{

class Drawable;

class Device;

namespace engine
{

bool Initialize();
void Run();
void Terminate();

void Inspect(std::vector<Drawable const *>);

} // namespace engine

} // namespace wsp

#endif
