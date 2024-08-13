#version 430

layout (location = 0) in vec2 vertexPosition;
layout (location = 1) in vec2 vertexSpeedIn;

uniform float sf;
uniform float totalVMax;

out vec2 vertexSpeed;

void main() {
    vertexSpeed = vec2(vertexSpeedIn.x, vertexSpeedIn.y);
    gl_Position = vec4(vertexPosition.x / sf, vertexPosition.y / sf, 0.0, 1.0);
}