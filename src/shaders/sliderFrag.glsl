#version 430

layout(location = 0) in vec2 vertexPosition;

uniform vec3 colourIn;

out vec4 colour;

void main() {
    colour = vec4(colourIn, 1.0);
}