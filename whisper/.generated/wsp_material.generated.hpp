#ifndef WFROST_GENERATED_wsp_material
#define WFROST_GENERATED_wsp_material

#include <frost.hpp>
#include <wsp_devkit.hpp>

 
#define WCLASS_BODY$Material() friend struct frost::Meta<wsp::Material>;

#undef WGENERATED_META_DATA
#define WGENERATED_META_DATA() \
using namespace wsp;template <> struct frost::Meta<wsp::Material> { static constexpr char const * name = "Material"; static constexpr frost::WhispUsage usage =frost::WhispUsage::eClass; using Type =wsp::Material;static constexpr auto fields = std::make_tuple(frost::Field<Type,TextureID>{"Albedo Texture", &Type::_albedoTexture,Edit::eInput,0.f,0.f,.01,"%.3f"},frost::Field<Type,TextureID>{"Normal Texture", &Type::_normalTexture,Edit::eInput,0.f,0.f,.01,"%.3f"},frost::Field<Type,TextureID>{"Metallic Roughness Texture", &Type::_metallicRoughnessTexture,Edit::eInput,0.f,0.f,.01,"%.3f"},frost::Field<Type,TextureID>{"Occlusion Texture", &Type::_occlusionTexture,Edit::eInput,0.f,0.f,.01,"%.3f"},frost::Field<Type,TextureID>{"Specular Texture", &Type::_specularTexture,Edit::eInput,0.f,0.f,.01,"%.3f"},frost::Field<Type,glm::vec3>{"Albedo Color", &Type::_albedoColor,eColor,0.f,0.f,.01,"%.3f"},frost::Field<Type,glm::vec3>{"Fresnel Color", &Type::_fresnelColor,eColor,0.f,0.f,.01,"%.3f"},frost::Field<Type,glm::vec3>{"Specular Color", &Type::_specularColor,eColor,0.f,0.f,.01,"%.3f"},frost::Field<Type,float>{"Roughness", &Type::_roughness,eSlider,0.f,1.f,.01,"%.3f"},frost::Field<Type,float>{"Metallic", &Type::_metallic,eSlider,0.f,1.f,.01,"%.3f"},frost::Field<Type,float>{"Anisotropy", &Type::_anisotropy,eSlider,0.f,1.f,.01,"%.3f"},frost::Field<Type,float>{"Ior", &Type::_ior,eSlider,0.f,100.f,.01,"%.3f"});static constexpr bool hasRefresh = false;};

#endif
