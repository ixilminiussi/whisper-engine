#ifndef WSP_MESH
#define WSP_MESH

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <vulkan/vulkan.hpp>

#include <cgltf.h>

#include <vector>

namespace wsp
{

class Mesh
{
  public:
    struct Vertex
    {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec3 tangent;
        glm::vec3 color;
        glm::vec3 uv; // materialID on z

        static vk::PipelineVertexInputStateCreateInfo GetVertexInputInfo();

        bool operator==(Vertex const &other) const;
    };

    struct Primitive
    {
        uint32_t materialID;
        uint32_t indexCount;
        uint32_t indexOffset;
        uint32_t vertexCount;
        uint32_t vertexOffset;
    };

    Mesh(class Device const *, class cgltf_mesh const *mesh);
    ~Mesh();

    Mesh(Mesh const &) = delete;
    Mesh &operator=(Mesh const &) = delete;

    void Free(class Device const *);

    void BindAndDraw(vk::CommandBuffer commandBuffer);

  private:
    static cgltf_accessor const *FindAccessor(cgltf_primitive const *prim, cgltf_attribute_type type);

    std::string _filepath;

    std::vector<Primitive> _primitives;

    vk::Buffer _vertexBuffer;
    vk::DeviceMemory _vertexDeviceMemory;

    vk::Buffer _indexBuffer;
    vk::DeviceMemory _indexDeviceMemory;

    bool _freed;
};

} // namespace wsp

#endif
