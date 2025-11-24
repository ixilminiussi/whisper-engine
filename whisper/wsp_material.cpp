#include <wsp_material.hpp>

#include <wsp_devkit.hpp>
#include <wsp_texture.hpp>

#include <cgltf.h>

using namespace wsp;

Material::Material(cgltf_material const *material, cgltf_texture const *texture,
                   std::vector<std::shared_ptr<Texture>> textures)
{
}
