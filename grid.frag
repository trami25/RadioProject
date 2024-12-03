#version 330 core

out vec4 outColor;

void main() {
    // Dimenzije celija mreže
    float gridSize = 20.0;

    // Provera da li je piksel unutar linije mreze
    float lineWidth = 1.5;
    float xMod = mod(gl_FragCoord.x, gridSize);
    float yMod = mod(gl_FragCoord.y, gridSize);
    float gridLine = (xMod < lineWidth || yMod < lineWidth) ? 1.0 : 0.0;

    // Boja rešetke (poluprovidna crna)
    float alpha = 0.5; // Podesavanje providnosti
    vec3 gridColor = vec3(0.0, 0.0, 0.0); // Crna boja

    // Kombinacija boje i providnosti
    outColor = vec4(gridColor * gridLine, alpha * gridLine);
}
