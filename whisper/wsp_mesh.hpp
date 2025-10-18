#ifndef WSP_MESH
#define WSP_MESH

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <wsp_drawable.hpp>

#include <vulkan/vulkan.hpp>

#include <cgltf.h>

#include <vector>

namespace wsp
{

class Mesh : public Drawable
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

    virtual void Bind(vk::CommandBuffer) const override;
    virtual void Draw(vk::CommandBuffer) const override;

    // if you had to fit the model in a sphere, how large the radius should be
    float GetRadius() const;

  private:
    static cgltf_accessor const *FindAccessor(cgltf_primitive const *prim, cgltf_attribute_type type);

    std::string _filepath;

    std::vector<Primitive> _primitives;

    vk::Buffer _vertexBuffer;
    vk::DeviceMemory _vertexDeviceMemory;

    vk::Buffer _indexBuffer;
    vk::DeviceMemory _indexDeviceMemory;

    float _radius;

    bool _freed;
};

} // namespace wsp

#endif
