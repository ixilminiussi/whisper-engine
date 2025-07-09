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

    Model(class Device const *, std::string const &filepath);
    ~Model();

    Model(Model const &) = delete;
    Model &operator=(Model const &) = delete;

    void Free();

    void Bind(vk::CommandBuffer commandBuffer);
    void Draw(vk::CommandBuffer commandBuffer);

  private:
    static void loadFromFile(std::string const &filepath, std::vector<Vertex> *vertices,
                             std::vector<uint32_t> *indices);

    std::string _filepath;

    vk::Buffer _vertexBuffer;
    uint32_t _vertexCount;

    bool _hasIndexBuffer;
    vk::Buffer _indexBuffer;
    uint32_t _indexCount;

    bool _freed;
};

} // namespace wsp

#endif
