#version 330 core

out vec4 outColor;
uniform vec3 color; // Uniform za prosleđivanje boje

void main() {
    outColor = vec4(color, 1.0); // Koristimo uniform boju sa alfa vrednošću 1.0
}
