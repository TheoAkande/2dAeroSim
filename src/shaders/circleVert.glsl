#version 430

layout (location = 0) in vec2 vertexPosition;

uniform vec3 colourIn;
uniform vec2 centre;
uniform float radiusX;
uniform float radiusY;

out vec2 fragPos;

void main() {
    gl_Position = vec4(vertexPosition.x, vertexPosition.y, 0.0, 1.0);
    fragPos = vertexPosition;
}