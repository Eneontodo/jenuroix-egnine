#pragma once

// ─── Unlit (цвет из vertex.color) ────────────────────────────────────────────
inline constexpr const char* VERT_UNLIT = R"GLSL(
#version 460 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aUV;
layout(location=3) in vec4 aColor;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;

out vec4 vColor;
out vec2 vUV;

void main(){
    gl_Position = uProj * uView * uModel * vec4(aPos, 1.0);
    vColor = aColor;
    vUV    = aUV;
}
)GLSL";

inline constexpr const char* FRAG_UNLIT = R"GLSL(
#version 460 core
in vec4 vColor;
in vec2 vUV;

uniform sampler2D uTexture;
uniform vec4      uTint;

out vec4 FragColor;

void main(){
    vec4 tex = texture(uTexture, vUV);
    FragColor = tex * vColor * uTint;
}
)GLSL";

// ─── Lit (Lambert + Blinn-Phong) ─────────────────────────────────────────────
inline constexpr const char* VERT_LIT = R"GLSL(
#version 460 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aUV;
layout(location=3) in vec4 aColor;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;
uniform mat3 uNormalMatrix; // transpose(inverse(mat3(uModel)))

out vec3 vFragPos;
out vec3 vNormal;
out vec2 vUV;
out vec4 vColor;

void main(){
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    gl_Position   = uProj * uView * worldPos;
    vFragPos = worldPos.xyz;
    vNormal  = normalize(uNormalMatrix * aNormal);
    vUV      = aUV;
    vColor   = aColor;
}
)GLSL";

inline constexpr const char* FRAG_LIT = R"GLSL(
#version 460 core
in vec3 vFragPos;
in vec3 vNormal;
in vec2 vUV;
in vec4 vColor;

uniform sampler2D uTexture;
uniform bool  uHasTexture;  // true = real texture bound, false = white 1x1
uniform vec4  uTint;        // used ONLY when uHasTexture == false
uniform vec3  uLightDir;
uniform vec3  uLightColor;
uniform vec3  uAmbient;
uniform vec3  uCamPos;

out vec4 FragColor;

void main(){
    // When a real texture is present: sample it exactly as-is (no tint).
    // When no texture: use the tint color (MTL Kd or GameObject::color()).
    vec4 baseColor;
    if (uHasTexture) {
        baseColor = texture(uTexture, vUV);
    } else {
        baseColor = uTint;
    }

    // Discard fully transparent fragments
    if (baseColor.a < 0.01) discard;

    // Ambient
    vec3 ambient = uAmbient * baseColor.rgb;

    // Diffuse (Lambert)
    vec3  N    = normalize(vNormal);
    float diff = max(dot(N, normalize(uLightDir)), 0.0);
    vec3  diffuse = diff * uLightColor * baseColor.rgb;

    // Specular (Blinn-Phong)
    vec3  viewDir = normalize(uCamPos - vFragPos);
    vec3  halfDir = normalize(normalize(uLightDir) + viewDir);
    float spec    = pow(max(dot(N, halfDir), 0.0), 32.0);
    vec3  specular = spec * uLightColor * 0.3;

    vec3 result = ambient + diffuse + specular;
    FragColor   = vec4(result, baseColor.a);
}
)GLSL";

// ─── Skybox (cubemap) ─────────────────────────────────────────────────────────
inline constexpr const char* VERT_SKYBOX = R"GLSL(
#version 460 core
layout(location=0) in vec3 aPos;

uniform mat4 uView;
uniform mat4 uProj;

out vec3 vTexCoord;

void main(){
    vTexCoord    = aPos;
    vec4 pos     = uProj * mat4(mat3(uView)) * vec4(aPos, 1.0);
    gl_Position  = pos.xyww; // трюк z=w → depth=1.0 (дальняя плоскость)
}
)GLSL";

inline constexpr const char* FRAG_SKYBOX = R"GLSL(
#version 460 core
in vec3 vTexCoord;
uniform samplerCube uSkybox;
out vec4 FragColor;

void main(){
    FragColor = texture(uSkybox, vTexCoord);
}
)GLSL";
