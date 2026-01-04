#include "wsp_constants.hpp"
#include <wsp_node.hpp>

#include <wsp_assets_manager.hpp>
#include <wsp_mesh.hpp>

#include <cgltf.h>

using namespace wsp;

void Node::Draw(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout,
                class Transform const &transform) const
{
    Transform const combined = transform + _transform;

    if (_mesh != INVALID_ID)
    {
        AssetsManager::Get()->GetMesh(_mesh)->Draw(commandBuffer, pipelineLayout, combined);
    }

    for (Node const *node : _children)
    {
        node->BindAndDraw(commandBuffer, pipelineLayout, combined);
    }
}

void Node::Bind(vk::CommandBuffer commandBuffer) const
{
    if (_mesh != INVALID_ID)
    {
        AssetsManager::Get()->GetMesh(_mesh)->Bind(commandBuffer);
    }
}

Node *Node::BuildGlTF(cgltf_scene const *scene, cgltf_mesh const *pMesh, std::vector<MeshID> const &meshes)
{
    check(scene);
    check(pMesh);

    Transform transform{};
    MeshID mesh{INVALID_ID};
    std::vector<Node *> children{};
    children.reserve(scene->nodes_count);

    for (int i = 0; i < scene->nodes_count; i++)
    {
        Node *child = BuildGlTF(scene->nodes[i], pMesh, meshes);
        children.push_back(child);
    }

    return new Node{mesh, children, transform};
}

Node *Node::BuildGlTF(cgltf_node const *node, cgltf_mesh const *pMesh, std::vector<MeshID> const &meshes)
{
    check(node);
    check(pMesh);

    Transform transform{};
    MeshID mesh{INVALID_ID};
    std::vector<Node *> children{};
    children.reserve(node->children_count);

    for (int i = 0; i < node->children_count; i++)
    {
        Node *child = BuildGlTF(node->children[i], pMesh, meshes);
        children.push_back(child);
    }

    if (node->mesh)
    {
        uint32_t const meshID = node->mesh - pMesh;
        check(meshID < meshes.size());
        mesh = meshes.at(meshID);
    }

    if (node->has_translation)
    {
        transform.SetPosition(*(glm::vec3 *)&(node->translation));
    }
    if (node->has_scale)
    {
        transform.SetScale(*(glm::vec3 *)&(node->scale));
    }
    if (node->has_rotation)
    {
        transform.SetRotation(*(glm::quat *)&(node->rotation));
    }
    if (node->has_matrix)
    {
        transform.FromMatrix(*(glm::mat4 *)&(node->matrix));
    }

    return new Node{mesh, children, transform};
}

Node::Node(MeshID mesh, std::vector<Node *> const &children, Transform const &transform)
    : Drawable{}, _mesh{mesh}, _children{children}, _transform{transform}
{
}

Node::~Node()
{
    auto it = _children.begin();
    while (it != _children.end())
    {
        Node *node = *it;
        it = _children.erase(it);

        delete node;
    }
}
