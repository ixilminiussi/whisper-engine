#ifndef WSP_SCENE
#define WSP_SCENE

#include <wsp_drawable.hpp>
#include <wsp_transform.hpp>

#include <vector>

class cgltf_scene;
class cgltf_mesh;
class cgltf_node;

namespace wsp
{

class Scene : public Drawable
{
  public:
    virtual void Bind(vk::CommandBuffer) const override;
    virtual void Draw(vk::CommandBuffer, vk::PipelineLayout, class Transform const &) const override;

    static Scene *BuildGlTF(cgltf_scene const *, cgltf_mesh const *pMesh, std::vector<MeshID> const &meshes);

    Scene(std::vector<std::pair<Transform, MeshID>> const &drawList);
    ~Scene() = default;

  protected:
    std::vector<std::pair<Transform, MeshID>> _drawList;
};

} // namespace wsp

#endif
