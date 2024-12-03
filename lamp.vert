#version 330 core

layout(location = 0) in vec2 inPos;

void main() {
    gl_Position = vec4(inPos + vec2(0.0, 0.35), 0.0, 1.0); // Lampica je iznad tela radija
}
