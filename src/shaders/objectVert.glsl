#version 430

layout (location = 0) in vec2 vertexPosition;

uniform float sf;
uniform mat4 viewMat;
uniform vec3 colourIn;

void main() {
    gl_Position = viewMat * vec4(vertexPosition.x / sf, vertexPosition.y / sf, 0.0, 1.0);
}