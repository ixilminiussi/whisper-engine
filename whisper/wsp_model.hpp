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
        glm::vec3 normal;
        glm::vec3 tangent;
        glm::vec2 uv;
        int material;

        static vk::PipelineVertexInputStateCreateInfo GetVertexInputInfo();

        bool operator==(Vertex const &other) const;
    };

    Model(class Device const *, std::vector<Vertex> const &, std::vector<uint32_t> const &);
    Model(class Device const *, std::string const &filepath);
    ~Model();

    Model(Model const &) = delete;
    Model &operator=(Model const &) = delete;

    void Free(class Device const *);

    void Bind(vk::CommandBuffer commandBuffer);
    void Draw(vk::CommandBuffer commandBuffer);

  private:
    static void LoadFromFile(std::string const &filepath, std::vector<Vertex> *, std::vector<uint32_t> *);

    void CreateVertexBuffers(Device const *, std::vector<Vertex> const &);
    void CreateIndexBuffers(Device const *, std::vector<uint32_t> const &);

    std::string _filepath;

    vk::Buffer _vertexBuffer;
    vk::DeviceMemory _vertexDeviceMemory;
    uint32_t _vertexCount;

    bool _hasIndexBuffer;
    vk::Buffer _indexBuffer;
    vk::DeviceMemory _indexDeviceMemory;
    uint32_t _indexCount;

    bool _freed;
};

} // namespace wsp

#endif
