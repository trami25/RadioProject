#version 330 core

layout(location = 0) in vec2 inPos;
layout(location = 1) in vec2 inTexCoord;

uniform float uTime;
uniform float uIntensity;

out vec2 texCoord;

void main() {
    float scale = 1.0 + 0.05 * sin(uTime * 10.0) * uIntensity;
    gl_Position = vec4(inPos * scale, 0.0, 1.0);
    texCoord = inTexCoord;
}
