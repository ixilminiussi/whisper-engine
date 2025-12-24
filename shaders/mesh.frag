#version 450
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require

#include "pbr_lib.glsl"

layout(location = 0) in v_info i;

layout(location = 0) out vec4 out_color;

layout(set = 1, binding = 0) uniform sampler2D textures[];
layout(set = 2, binding = 0) uniform samplerCube cubemaps[];

#include "ubo.glsl"

#define ENV_MAP_MIP_LVL 14

layout(push_constant) uniform Push
{
    mat4 modelMatrix;
    mat4 normalMatrix;
}
push;

vec3 getNormal(in Material material, in vec2 uv, in mat3 tangentMatrix)
{
    int normalTexID = material.normalTex;
    vec3 normal =
        normalTexID != INVALID_ID ? tangentMatrix * (texture(textures[normalTexID], uv).rgb * 2.0 - 1.0) : i.w_normal;
    return normalize(normal);
}

vec3 getAlbedo(in Material material, in vec2 uv)
{
    int albedoTexID = material.albedoTex;
    return albedoTexID != INVALID_ID ? texture(textures[albedoTexID], uv).rgb : material.albedoColor;
}

vec3 getPBRParams(in Material material, in vec2 uv)
{
    int metallicRoughnessTexID = material.metallicRoughnessTex;

    vec3 params;
    params.r = material.roughness;
    params.g = material.metallic;

    if (metallicRoughnessTexID != INVALID_ID)
    {
        params.r *= texture(textures[metallicRoughnessTexID], uv).g;
        params.g *= texture(textures[metallicRoughnessTexID], uv).b;
    }

    return params;
}

float getOcclusion(in Material material, in vec2 uv)
{
    float occlusion = 1.0;

    int occlusionTexID = material.occlusionTex;

    if (occlusionTexID != INVALID_ID)
    {
        occlusion = saturate(texture(textures[occlusionTexID], uv).r);
    }

    return occlusion;
}

void main()
{
    vec2 uv = i.uv.xy;
    int materialID = int(i.uv.z);

    if (materialID == INVALID_ID)
    {
        out_color = vec4(1., 0., 0., 1.);
        return;
    }

    Material material = ubo.materials[materialID];

    // TEXTURE SAMPLES
    vec3 albedo = getAlbedo(material, uv);
    float occlusion = getOcclusion(material, uv);

    // PBR PARAMETERS
    vec3 PBRParams = getPBRParams(material, uv);

    float roughness = PBRParams.r;
    float metallic = PBRParams.g;

    mat3 tangentMatrix = transpose(mat3(normalize(i.w_tangent), normalize(i.w_bitangent), normalize(i.w_normal)));
    // FOUNDATION parameters
    vec3 V = normalize(ubo.camera.position.xyz - i.w_position); // view vec

    vec3 N = getNormal(material, uv, tangentMatrix); // normal

    // DOT PRODUCTS
    float NdotV = max(dot(N, V), 0.0);
    float NdotUp = saturate(dot(N, vec3(0, 1, 0)));

    vec3 R = reflect(-V, N);

    vec4 irradiance = texture(cubemaps[1], N); // temporary
    vec3 specular = texture(cubemaps[0], R).rgb * metallic;
    vec3 diffuse = albedo * max(1.0 - metallic, EPS) * irradiance.rgb * irradiance.a;

    vec3 color = specular + diffuse;

    out_color = vec4(color, 1.0);
    return;
}
