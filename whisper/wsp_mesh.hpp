#ifndef WSP_MESH
#define WSP_MESH

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <wsp_drawable.hpp>
#include <wsp_typedefs.hpp>
#include <wsp_types/slot_map.hpp>

#include <vulkan/vulkan.hpp>

#include <vector>

class cgltf_mesh;
class cgltf_material;

namespace wsp
{

class Mesh : public Drawable
{
  public:
    struct Vertex
    {
        glm::vec4 tangent;
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec3 color;
        glm::vec2 uv;

        static vk::PipelineVertexInputStateCreateInfo GetVertexInputInfo();

        bool operator==(Vertex const &other) const;
    };

    struct PushData
    {
        glm::mat4 modelMatrix;
        glm::mat4 normalMatrix; // pushing a mat3 causes misalignments
                                // material id on normalMatrix[2][2]
    };

    struct Primitive
    {
        MaterialID material;
        uint32_t indexCount;
        uint32_t indexOffset;
        uint32_t vertexCount;
        uint32_t vertexOffset;
    };

    static Mesh *BuildGlTF(class Device const *, cgltf_mesh const *, cgltf_material const *pMaterial,
                           std::vector<MaterialID> const &materials, bool recenter = false);

    Mesh(class Device const *, std::vector<Vertex> const &vertices, std::vector<uint32_t> const &indices,
         std::vector<Primitive> const &primitives, std::string const &name = "");
    ~Mesh();

    Mesh(Mesh const &) = delete;
    Mesh &operator=(Mesh const &) = delete;

    virtual void Bind(vk::CommandBuffer) const override;
    virtual void Draw(vk::CommandBuffer, vk::PipelineLayout, class Transform const &) const override;

    void PushConstant(Primitive const &, class Transform const &, vk::CommandBuffer, vk::PipelineLayout) const;

  private:
    std::string _name;

    std::vector<Primitive> _primitives;

    vk::Buffer _vertexBuffer;
    vk::DeviceMemory _vertexDeviceMemory;

    vk::Buffer _indexBuffer;
    vk::DeviceMemory _indexDeviceMemory;
};

} // namespace wsp

#endif
