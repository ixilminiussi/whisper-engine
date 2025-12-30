#ifndef WSP_NODE
#define WSP_NODE

#include <wsp_devkit.hpp>
#include <wsp_drawable.hpp>

#include ".generated/wsp_node.generated.hpp"

class cgltf_node;
class cgltf_mesh;

namespace wsp
{

WCLASS()
class Node : public Drawable
{
  public:
    Node(MeshID, std::vector<class Node *> const &, Transform const &transform);
    ~Node();

    virtual void Draw(vk::CommandBuffer, vk::PipelineLayout, class Transform const &) const override;
    virtual void Bind(vk::CommandBuffer) const override;

    static Node *BuildGlTF(cgltf_node const *, cgltf_mesh const *pMesh, std::vector<MeshID> const &meshes);

    WCLASS_BODY$Node();

  protected:
    WPROPERTY()
    Transform _transform;

    WPROPERTY()
    std::vector<class Node *> _children;
    MeshID _mesh;
};

} // namespace wsp

WGENERATED_META_DATA()

#endif
