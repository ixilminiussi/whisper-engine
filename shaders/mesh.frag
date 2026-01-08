#version 450
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require

#include "pbr_lib.glsl"

layout(location = 0) in v_info i;

layout(location = 0) out vec4 out_color;

layout(set = 1, binding = 0) uniform sampler2D sShadowMap;
layout(set = 2, binding = 0) uniform sampler2D sPrepass;
layout(set = 3, binding = 0) uniform sampler2D sTextures[];
layout(set = 4, binding = 0) uniform sampler2D sNoises[]; // 0 regular noise
layout(set = 5, binding = 0) uniform samplerCube sCubemaps[];

#include "ubo.glsl"

#define ENV_MAP_MIP_LVL 14

layout(push_constant) uniform Push
{
    mat4 modelMatrix;
    mat4 normalMatrix;
}
push;

vec4 getSky(in vec3 ray, in float mipLevel)
{
    int skyboxTexID = ubo.light.skyboxTex;
    return skyboxTexID != INVALID_ID ? texture(sCubemaps[skyboxTexID], ray, mipLevel * 7.)
                                     : vec4(ubo.light.sun.color.rgb, 0.);
}

vec4 getIrradiance(in vec3 ray)
{
    int irradianceTexID = ubo.light.irradianceTex;
    return irradianceTexID != INVALID_ID ? texture(sCubemaps[irradianceTexID], ray) : vec4(ubo.light.sun.color.rgb, 0.);
}

vec3 getNormal()
{
    vec2 uv = i.c_position.xy / i.c_position.w;
    uv = uv / 2. + .5;
    return normalize(texture(sPrepass, uv).rgb);
}

vec3 getSpecular(in Material material, in vec2 uv)
{
    int specularTexID = material.specularTex;
    return specularTexID != INVALID_ID ? texture(sTextures[specularTexID], uv).rgb : material.specularColor;
}

vec3 getAlbedo(in Material material, in vec2 uv)
{
    int albedoTexID = material.albedoTex;
    return albedoTexID != INVALID_ID ? texture(sTextures[albedoTexID], uv).rgb : material.albedoColor;
}

vec3 getPBRParams(in Material material, in vec2 uv)
{
    int metallicRoughnessTexID = material.metallicRoughnessTex;

    vec3 params;
    params.r = material.roughness;
    params.g = material.metallic;

    if (metallicRoughnessTexID != INVALID_ID)
    {
        params.r = texture(sTextures[metallicRoughnessTexID], uv).g;
        params.g = texture(sTextures[metallicRoughnessTexID], uv).b;
    }

    return params;
}

vec3 getRandom()
{
    vec2 uv = i.c_position.xy / i.c_position.w;
    uv = uv / 2. + .5;
    vec2 screenSize = textureSize(sPrepass, 0);
    vec2 noiseSize = textureSize(sNoises[0], 0);
    return texture(sNoises[0], uv * screenSize / noiseSize).xyz * 2.0 - 1.0;
}

float computeSSAO(vec3 random, float ssaoRadius)
{
    random = normalize(random);

    const vec3 SSAO_DIRECTIONS[16] = {
        {0.35, 0.20, 0.85},  {-0.30, -0.20, 0.90}, {0.15, -0.35, 0.92}, {-0.10, 0.45, 0.88},
        {0.50, -0.10, 0.75}, {-0.45, 0.15, 0.70},  {0.20, 0.60, 0.65},  {-0.60, -0.25, 0.60},
        {0.65, 0.40, 0.55},  {-0.70, 0.30, 0.50},  {0.25, -0.70, 0.45}, {-0.75, -0.35, 0.40},
        {0.80, 0.10, 0.35},  {-0.20, 0.75, 0.30},  {0.10, -0.80, 0.25}, {-0.85, 0.00, 0.20}};

    float radius = ssaoRadius * (-i.v_position.z / 5.0);

    float occlusion = 0.;

    const float bias = radius * .005f;

    // random rotation
    vec3 v_normal = normalize(i.v_normal);
    vec3 v_tangent = normalize(random - v_normal * dot(random, v_normal));
    vec3 v_bitangent = cross(v_tangent, v_normal);
    mat3 TBN = mat3(v_tangent, v_bitangent, v_normal);

    float total = 0.f;
    vec3 bentNormal = vec3(0.f);
    for (int j = 0; j < 16; j++)
    {
        // sample point in direction
        vec3 v_sampleDirection = TBN * SSAO_DIRECTIONS[j];
        if (dot(v_sampleDirection, i.v_normal) < 0.f)
        {
            continue;
        }
        total += 1.f;

        vec3 v_sample = i.v_position + v_sampleDirection * radius;
        float v_sampleDepth = -v_sample.z;

        // compare to the true point at xy according to depth texture
        vec4 c_sample = ubo.camera.projection * vec4(v_sample, 1.);
        c_sample /= c_sample.w;
        float v_trueDepth = texture(sPrepass, c_sample.xy * .5 + .5).a;
        vec3 v_truePos = vec3(v_sample.xy, -v_trueDepth);

        // compare "collision" point to center
        float dist = length(v_truePos - v_sample);
        float rangeCheck = 1.0 - smoothstep(0.0, radius, dist);

        // bentNormal = v_sampleDirection * rangeCheck;
        occlusion += (v_trueDepth < v_sampleDepth - bias ? 1.0 : 0.0) * rangeCheck;
    }

    // return normalize(bentNormal);
    return 1. - occlusion / total;
}

float getOcclusion(in Material material, in vec2 uv)
{
    float occlusion = 1.;

    int occlusionTexID = material.occlusionTex;

    if (occlusionTexID != INVALID_ID)
    {
        occlusion = saturate(texture(sTextures[occlusionTexID], uv).r);
    }

    return occlusion;
}

vec3 computeFresnel(in float metallic, in vec3 albedo, in float NdotV, vec3 specular)
{
    vec3 F0 = mix(specular, albedo, metallic);
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

float isOccluded(in vec3 sc_position)
{
    vec2 uv = 0.5 * sc_position.xy + 0.5;
    float depth = texture(sShadowMap, uv).r;

    if (depth < sc_position.z - 0.025)
    {
        return 0.0f;
    }

    return 1.0f;
}

void main()
{
    int materialID = int(i.materialID);

    if (materialID == INVALID_ID)
    {
        out_color = vec4(1., 0., 0., 1.);
        return;
    }

    Material material = ubo.materials[materialID];

    vec3 lightColor = ubo.light.sun.color.rgb * ubo.light.sun.color.a;

    // TEXTURE SAMPLES
    vec3 albedo = getAlbedo(material, i.uv);
    float occlusion = getOcclusion(material, i.uv); // * computeSSAO(getRandom(), .1);
    vec3 specular = getSpecular(material, i.uv);

    // PBR PARAMETERS
    vec3 PBRParams = getPBRParams(material, i.uv);

    float roughness = PBRParams.r;
    float metallic = PBRParams.g;
    // FOUNDATION parameters
    vec3 V = -normalize(i.w_ray);
    vec3 N = getNormal(); // normal
    vec3 L = -normalize(ubo.light.sun.direction);
    vec3 H = normalize(V + L);
    vec3 R = reflect(-V, N);

    // DOT PRODUCTS
    float NdotV = max(dot(N, -V), EPS);
    float NdotUp = saturate(dot(N, vec3(0., 1., 0.)));
    float NdotL = max(dot(N, L), EPS);
    float NdotH = max(dot(N, H), 0.);

    vec3 F = computeFresnel(metallic, albedo, NdotV, specular);
    float D = computeDGGX(roughness, NdotH);
    float G = computeGGX(roughness, NdotL, NdotV);

    vec3 kS = F;
    vec3 kD = (1. - kS) * (1. - metallic);

    // IBL
    // vec3 bentNormal = computeSSAO(getRandom(), .05);
    // bentNormal = (ubo.camera.inverseView * vec4(bentNormal, 0.)).xyz;
    vec4 irradiance = getIrradiance(-N);

    vec3 specularIBL = getSky(-R, roughness).rgb * kS;
    vec3 diffuseIBL = (albedo * irradiance.rgb * irradiance.a) * kD * occlusion;
    vec3 IBLColor = specularIBL + diffuseIBL;

    // Direct lighting
    vec3 Ld = kD * (albedo / PI) * NdotL * lightColor;
    vec3 Ls = kS * ((D * G * F) / (4. * NdotL * NdotV)) * lightColor * NdotL;
    vec3 directColor = (Ld + Ls) * isOccluded(i.sc_position);

    out_color = vec4(directColor + IBLColor, 1.);
    // out_color = vec4(bentNormal, 1.);
}
