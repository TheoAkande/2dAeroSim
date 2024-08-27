#version 430

uniform vec3 colourIn;
uniform vec2 centre;
uniform float radiusX;
uniform float radiusY;

in vec2 fragPos;

out vec4 colour;

void main() {
    float dx = (fragPos.x - centre.x) * (fragPos.x - centre.x) / (radiusX * radiusX);
    float dy = (fragPos.y - centre.y) * (fragPos.y - centre.y) / (radiusY * radiusY);
    bool within = dx + dy <= 1.0;
    if (within) {
        colour = vec4(colourIn, 1.0);
    } else {
        colour = vec4(0.0, 0.0, 0.0, 0.0);
    }
}