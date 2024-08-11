#version 430

layout(location = 0) in vec2 vertexPosition;

uniform mat4 projection;
uniform mat4 model;

out vec4 colour;

void main() {
    colour = vec4(1.0, 0.0, 0.0, 1.0);
}