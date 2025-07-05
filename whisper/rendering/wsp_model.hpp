#ifndef WSP_MODEL
#define WSP_MODEL

// lib
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <vulkan/vulkan.hpp>

// std
#include <vector>

namespace wsp
{

class Model
{
  public:
    struct Vertex
    {
        glm::vec3 position;
        glm::vec3 color;
        glm::vec3 normal;
        glm::vec2 uv;
        glm::vec3 tangent;

        static vk::PipelineVertexInputStateCreateInfo GetVertexInputInfo();

        bool operator==(const Vertex &other) const;
    };

    Model(class Device *);
    ~Model();

    Model(const Model &) = delete;
    Model &operator=(const Model &) = delete;

    void Free();

  private:
    bool _freed;
};

} // namespace wsp

#endif
