#version 330 core

out vec4 outColor;

uniform float uTime;

void main() {
    // Interpolacija boje od bele ka narandzastoj i nazad
    float intensity = (sin(uTime * 2.0) + 1.0) / 2.0; // Vrednost izmedju 0 i 1
    vec3 white = vec3(1.0, 1.0, 1.0); // Bela boja
    vec3 orange = vec3(1.0, 0.5, 0.0); // Narandzasta boja
    vec3 currentColor = mix(white, orange, intensity);

    outColor = vec4(currentColor, 1.0); // Dodajemo alfa kanal
}
