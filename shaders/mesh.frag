#version 450
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require

#include "pbr_lib.glsl"

layout(location = 0) in v_info i;

layout(location = 0) out vec4 out_color;

layout(set = 1, binding = 0) uniform sampler2D textures[];
layout(set = 2, binding = 0) uniform sampler2D engineTextures[];
layout(set = 3, binding = 0) uniform samplerCube cubemaps[];

#include "ubo.glsl"

#define ENV_MAP_MIP_LVL 14

layout(push_constant) uniform Push
{
    mat4 modelMatrix;
    mat4 normalMatrix;
}
push;

vec4 getSky(in vec3 ray)
{
    int skyboxTexID = ubo.light.skyboxTex;
    return skyboxTexID != INVALID_ID ? texture(cubemaps[skyboxTexID], ray) : vec4(ubo.light.sun.color.rgb, 0.);
}

vec4 getIrradiance(in vec3 ray)
{
    int irradianceTexID = ubo.light.irradianceTex;
    return irradianceTexID != INVALID_ID ? texture(cubemaps[irradianceTexID], ray) : vec4(ubo.light.sun.color.rgb, 0.);
}

vec3 getNormal(in Material material, in vec2 uv, in mat3 tangentMatrix)
{
    int normalTexID = material.normalTex;
    vec3 normal =
        normalTexID != INVALID_ID ? tangentMatrix * (texture(textures[normalTexID], uv).rgb * 2. - 1.) : i.w_normal;
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
        params.r = texture(textures[metallicRoughnessTexID], uv).g;
        params.g = texture(textures[metallicRoughnessTexID], uv).b;
    }

    return params;
}

float getOcclusion(in Material material, in vec2 uv)
{
    float occlusion = 1.;

    int occlusionTexID = material.occlusionTex;

    if (occlusionTexID != INVALID_ID)
    {
        occlusion = saturate(texture(textures[occlusionTexID], uv).r);
    }

    return occlusion;
}

vec3 computeFresnel(in float metallic, in vec3 albedo, in float NdotV)
{
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    return F0 + (1. - F0) * pow(1. - NdotV, 5.);
}

float computeDGGX(in float roughness, in float NdotH)
{
    float a2 = pow(roughness, 4.);

    float denom = (NdotH * NdotH) * (a2 - 1.) + 1.;
    return a2 / (PI * denom * denom);
}

float computeGGX1(in float a2, in float NdotX)
{
    return (2. * NdotX) / (NdotX + sqrt(a2 + (1. - a2) * pow(NdotX, 2.)));
}

float computeGGX(in float roughness, in float NdotL, in float NdotV)
{
    float a2 = pow(roughness, 4.);

    return computeGGX1(a2, NdotL) * computeGGX1(a2, NdotV);
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

    vec3 lightColor = ubo.light.sun.color.rgb * ubo.light.sun.color.a;

    // TEXTURE SAMPLES
    vec3 albedo = getAlbedo(material, uv);
    float occlusion = getOcclusion(material, uv);

    // PBR PARAMETERS
    vec3 PBRParams = getPBRParams(material, uv);

    float roughness = PBRParams.r;
    float metallic = PBRParams.g;

    mat3 tangentMatrix = mat3(normalize(i.w_tangent), normalize(i.w_bitangent), normalize(i.w_normal));
    // FOUNDATION parameters
    vec3 V = -normalize(i.w_ray);
    vec3 N = getNormal(material, uv, tangentMatrix); // normal
    vec3 L = -normalize(ubo.light.sun.direction);
    vec3 H = normalize(V + L);
    vec3 R = reflect(-V, N);

    // DOT PRODUCTS
    float NdotV = max(dot(N, -V), EPS);
    float NdotUp = saturate(dot(N, vec3(0., 1., 0.)));
    float NdotL = max(dot(N, L), EPS);
    float NdotH = max(dot(N, H), 0.);

    vec3 F = computeFresnel(metallic, albedo, NdotV);
    float D = computeDGGX(roughness, NdotH);
    float G = computeGGX(roughness, NdotL, NdotV);

    vec3 kS = F;
    vec3 kD = (1. - kS) * (1. - metallic);

    // IBL
    vec4 irradiance = getIrradiance(-N);
    // vec2 brdf = texture(engineTextures[0], vec2(NdotV, roughness)).rg;

    // placeholder, will use mip maps
    vec3 specularIBL = mix(getSky(-R).rgb, getIrradiance(-R).rgb, roughness) * kS;
    // specularIBL *= (brdf.r + brdf.g);

    vec3 diffuseIBL = (albedo * irradiance.rgb * irradiance.a) * kD * occlusion;

    vec3 Ld = kD * (albedo / PI) * NdotL * lightColor;
    vec3 Ls = kS * ((D * G * F) / (4. * NdotL * NdotV)) * lightColor * NdotL;

    vec3 IBLColor = specularIBL + diffuseIBL;
    vec3 directColor = Ld + Ls;

    // Direct lighting

    out_color = vec4(IBLColor + directColor, 1.); // debug purposes
    return;
}
