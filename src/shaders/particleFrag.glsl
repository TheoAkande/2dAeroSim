#version 430

in vec2 vertexSpeed;

uniform float sf;
uniform float totalVMax;

out vec4 colour;

void main() {
    colour = vec4(abs(vertexSpeed.x) / totalVMax, abs(vertexSpeed.y) / totalVMax, 1.0, 1.0);
}