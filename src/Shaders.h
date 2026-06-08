// Shaders.h - Embedded GLSL source (OpenGL 3.3 core).
// Kept simple (single directional + point light, per-vertex color, emissive flash)
// so it runs smoothly on integrated GPUs (Intel HD / AMD APU) with no NVIDIA needed.
#pragma once

// ---------- 3D lit shader ----------
static const char* VS_LIT = R"(#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec3 aColor;
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;
out vec3 vNormal;
out vec3 vWorld;
out vec3 vColor;
void main(){
    vec4 world = uModel * vec4(aPos,1.0);
    vWorld = world.xyz;
    vNormal = mat3(uModel) * aNormal;
    vColor = aColor;
    gl_Position = uProj * uView * world;
}
)";

static const char* FS_LIT = R"(#version 330 core
in vec3 vNormal;
in vec3 vWorld;
in vec3 vColor;
out vec4 FragColor;
uniform vec3 uViewPos;
uniform vec3 uLightDir;     // directional (sun/moon)
uniform vec3 uLightColor;
uniform vec3 uAmbient;
uniform vec3 uEmissive;
uniform float uEmissiveAmt;
uniform vec3 uPointPos;     // player aura / spell light
uniform vec3 uPointColor;
uniform float uPointStrength;
uniform float uFogDensity;
uniform vec3 uFogColor;
// extra point lights (braziers etc.)
uniform int  uNumLights;
uniform vec3 uLightPos[6];
uniform vec3 uLightCol[6];

void main(){
    vec3 N = normalize(vNormal);
    vec3 V = normalize(uViewPos - vWorld);

    // key directional light (soft wrap for richer shading, not too dark)
    vec3 L = normalize(-uLightDir);
    float ndl = dot(N,L);
    float diff = ndl*0.4+0.6;          // gentle wrap, keeps shadows lit
    vec3 H = normalize(L+V);
    float spec = pow(max(dot(N,H),0.0), 32.0) * 0.35;

    // sky/ground hemispheric ambient for depth
    float up = N.y*0.5+0.5;
    vec3 skyAmb = mix(uAmbient*0.9, uAmbient*1.5, up);

    vec3 base = vColor;
    vec3 color = skyAmb*base + diff*uLightColor*base + spec*uLightColor;

    // player aura point light
    vec3 PL = uPointPos - vWorld;
    float pdist = length(PL);
    float patt = uPointStrength / (1.0 + 0.15*pdist + 0.04*pdist*pdist);
    color += patt * uPointColor * base * max(dot(N, normalize(PL)),0.0);

    // extra point lights (braziers) - warm pools of light
    for (int i=0;i<uNumLights;i++){
        vec3 d = uLightPos[i] - vWorld;
        float dl = length(d);
        float att = 1.0/(1.0 + 0.12*dl + 0.05*dl*dl);
        color += att * uLightCol[i] * base * (max(dot(N, normalize(d)),0.0)*0.8+0.2);
    }

    // rim light for silhouette pop
    float rim = pow(1.0 - max(dot(N,V),0.0), 3.0) * 0.25;
    color += rim * uLightColor;

    // emissive flash / glow
    color = mix(color, uEmissive, clamp(uEmissiveAmt,0.0,1.0));

    // exponential fog for depth/atmosphere
    float d2 = length(uViewPos - vWorld);
    float fog = 1.0 - exp(-uFogDensity*uFogDensity*d2*d2);
    color = mix(color, uFogColor, clamp(fog,0.0,1.0));

    // gentle filmic tone curve + slight saturation boost
    color = color/(color+vec3(0.85))*1.85;
    float lum = dot(color, vec3(0.299,0.587,0.114));
    color = mix(vec3(lum), color, 1.15);

    FragColor = vec4(max(color,0.0), 1.0);
}
)";

// ---------- Particle / billboard shader (spells, effects) ----------
static const char* VS_PARTICLE = R"(#version 330 core
layout(location=0) in vec2 aQuad;     // -0.5..0.5
layout(location=1) in vec3 aCenter;   // instance world pos
layout(location=2) in vec4 aColor;    // instance rgba
layout(location=3) in float aSize;    // instance size
uniform mat4 uView;
uniform mat4 uProj;
out vec4 vColor;
out vec2 vUV;
void main(){
    vec3 camRight = vec3(uView[0][0], uView[1][0], uView[2][0]);
    vec3 camUp    = vec3(uView[0][1], uView[1][1], uView[2][1]);
    vec3 world = aCenter + (camRight*aQuad.x + camUp*aQuad.y) * aSize;
    vColor = aColor;
    vUV = aQuad*2.0;
    gl_Position = uProj * uView * vec4(world,1.0);
}
)";

static const char* FS_PARTICLE = R"(#version 330 core
in vec4 vColor;
in vec2 vUV;
out vec4 FragColor;
void main(){
    float r = length(vUV);
    float a = smoothstep(1.0, 0.0, r);     // soft round sprite
    FragColor = vec4(vColor.rgb, vColor.a * a);
}
)";

// ---------- 2D UI shader (solid colored quads) ----------
static const char* VS_UI = R"(#version 330 core
layout(location=0) in vec2 aPos;   // pixel coords
layout(location=1) in vec4 aColor;
uniform vec2 uScreen;
out vec4 vColor;
void main(){
    vec2 ndc = vec2(aPos.x/uScreen.x*2.0-1.0, 1.0-aPos.y/uScreen.y*2.0);
    vColor = aColor;
    gl_Position = vec4(ndc,0.0,1.0);
}
)";

static const char* FS_UI = R"(#version 330 core
in vec4 vColor;
out vec4 FragColor;
void main(){ FragColor = vColor; }
)";

// ---------- Line shader (lock-on links, ground decals edges) ----------
static const char* VS_LINE = R"(#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec4 aColor;
uniform mat4 uView;
uniform mat4 uProj;
out vec4 vColor;
void main(){
    vColor = aColor;
    gl_Position = uProj*uView*vec4(aPos,1.0);
}
)";

static const char* FS_LINE = R"(#version 330 core
in vec4 vColor;
out vec4 FragColor;
void main(){ FragColor = vColor; }
)";
