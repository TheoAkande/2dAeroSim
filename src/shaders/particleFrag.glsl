#version 430

in vec2 vertexSpeed;

uniform float sf;
uniform float totalVMax;

out vec4 colour;

void main() {
    colour = vec4(vertexSpeed.x / totalVMax, vertexSpeed.y / totalVMax, 0.5, 1.0);
}