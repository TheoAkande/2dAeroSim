#version 430

layout (location = 0) in vec2 vertexPosition;

uniform float sf;

void main() {
    gl_Position = vec4(vertexPosition.x / sf, vertexPosition.y / sf, 0.0, 1.0);
}