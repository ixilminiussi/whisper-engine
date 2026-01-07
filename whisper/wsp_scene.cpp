#include <wsp_scene.hpp>

#include <wsp_assets_manager.hpp>
#include <wsp_constants.hpp>
#include <wsp_mesh.hpp>

#include <cgltf.h>

using namespace wsp;

Scene *Scene::BuildGlTF(cgltf_scene const *scene, cgltf_mesh const *pMesh, std::vector<MeshID> const &meshes)
{
    check(scene);
    check(pMesh);

    std::vector<std::pair<Transform, MeshID>> drawList;

    for (int i = 0; i < scene->nodes_count; i++)
    {
        std::function<void(cgltf_node * node, Transform const &parent)> const explore =
            [&drawList, &meshes, &pMesh, &explore](cgltf_node *node, Transform const &parent) {
                Transform transform{};
                MeshID mesh{INVALID_ID};

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

                transform = parent + transform;

                if (node->mesh)
                {
                    uint32_t const meshID = node->mesh - pMesh;
                    check(meshID < meshes.size());
                    mesh = meshes.at(meshID);
                    drawList.emplace_back(transform, meshes.at(meshID));
                }

                for (int i = 0; i < node->children_count; i++)
                {
                    explore(node->children[i], transform);
                }
            };

        explore(scene->nodes[i], Transform{});
    }

    return new Scene(drawList);
}

Scene::Scene(std::vector<std::pair<Transform, MeshID>> const &drawList)
{
    _drawList.reserve(drawList.size());
    _drawList.assign(drawList.begin(), drawList.end());
}

void Scene::Bind(vk::CommandBuffer commandBuffer) const
{
}

void Scene::Draw(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, class Transform const &) const
{
    AssetsManager const *assetsManager = AssetsManager::Get();

    for (auto const &[transform, meshID] : _drawList)
    {
        Mesh const *mesh = assetsManager->GetMesh(meshID);

        if (mesh)
        {
            mesh->Bind(commandBuffer);
            mesh->Draw(commandBuffer, pipelineLayout, transform);
        }
    }
}
