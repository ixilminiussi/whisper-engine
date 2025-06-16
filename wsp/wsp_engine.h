#ifndef WSP_ENGINE_H
#define WSP_ENGINE_H

#include <vector>

namespace wsp
{

class Engine
{
  public:
    static Engine *Get();
    static void Kickstart();
    static void Shutdown();

    void Run();

    class Device *GetDevice();

  protected:
    void Initialize();
    void Deinitialize();

    Engine();
    ~Engine() = default;

    static Engine *_instance;

    class Device *_device;
    class Window *_window;
};

} // namespace wsp

#endif
