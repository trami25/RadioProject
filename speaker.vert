#version 330 core

layout(location = 0) in vec2 inPos;
uniform float uTime;
uniform float uIntensity;

void main() {
    // Vibracija zavisi od intenziteta (sliderValue)
    float scale = 1.0 + 0.05 * sin(uTime * 10.0) * uIntensity;

    gl_Position = vec4(inPos * scale, 0.0, 1.0);
}
