#version 330 core

layout(location = 0) in vec2 inPos;
uniform float uTime;

void main() {
    float scale = 1.0 + 0.05 * sin(uTime * 10.0); // Vibracije
    gl_Position = vec4(inPos * scale + vec2(0.0, 0.0), 0.0, 1.0); // Centriran krug
}
