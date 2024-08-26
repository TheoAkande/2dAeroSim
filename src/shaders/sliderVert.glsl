#version 430

layout (location = 0) in vec2 vertexPosition;

uniform vec3 colourIn;

void main() {
    gl_Position = vec4(vertexPosition.x, vertexPosition.y, 0.0, 1.0);
}