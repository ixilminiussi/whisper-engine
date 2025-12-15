#version 450
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require

#include "pbr_lib.glsl"

layout(location = 0) in v_info i;

layout(location = 0) out vec4 out_color;

layout(set = 1, binding = 0) uniform sampler2D textures[];

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
        params.r = saturate(max(material.roughness + EPS, texture(textures[metallicRoughnessTexID], uv).r));
        params.g = saturate(max(material.metallic + EPS, texture(textures[metallicRoughnessTexID], uv).g));
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

float D_GGX(float NdotH, float roughness)
{
    float r4 = pow(roughness, 4);
    float d = (NdotH * NdotH) * (r4 - 1.0) + 1.0;
    return r4 / (PI * d * d);
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float G_SchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float G_Smith(float NdotV, float NdotL, float roughness)
{
    return G_SchlickGGX(NdotV, roughness) * G_SchlickGGX(NdotL, roughness);
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
    vec3 L = normalize(ubo.light.sun.direction.xyz); // light vec

    vec3 V = normalize(ubo.camera.position.xyz - i.w_position); // view vec
    vec3 H = normalize(L + V);                                  // half vec

    vec3 N = getNormal(material, uv, tangentMatrix); // normal

    // DOT PRODUCTS
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    float NdotH = max(dot(N, H), 0.0);
    float VdotH = max(dot(V, H), 0.0);
    float NdotUp = saturate(dot(N, vec3(0, 1, 0)));

    // PBR
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 F = fresnelSchlick(VdotH, F0);
    float D = D_GGX(NdotH, roughness);
    float G = G_Smith(NdotV, NdotL, roughness);

    // diffuse
    vec3 diffuse = albedo / PI;

    vec3 kS = F;
    vec3 kD = (1.0 - kS) * (1.0 - metallic);

    // specular
    vec3 specular = (D * F * G) / max(4.0 * NdotV * NdotL, EPS);

    // ambient
    vec3 ambient = kD * diffuse * 0.1 * ubo.light.sun.color.rgb;

    // combination
    vec3 color = (kD * diffuse + specular) * ubo.light.sun.color.rgb * NdotL + ambient;

    out_color = vec4(N, 1.0);
    return;
}
