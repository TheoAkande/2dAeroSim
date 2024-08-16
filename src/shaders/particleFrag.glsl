#version 430

in vec2 vertexSpeed;

in vec4 colourOut;

uniform float sf;
uniform float totalVMax;

out vec4 colour;

void main() {
    colour = colourOut;
    // colour = vec4(vertexSpeed.x / totalVMax, vertexSpeed.y / totalVMax, 1.0, 1.0);
}