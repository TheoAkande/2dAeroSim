#version 430

layout (location = 0) in vec2 vertexPosition;
layout (location = 1) in vec2 vertexSpeedIn;
layout (location = 2) in float chunk;

uniform float sf;
uniform float totalVMax;

out vec2 vertexSpeed;

out vec4 colourOut;

void main() {

    vec4 colors[6];  // Define an array of vec4 with 5 elements

    // Assign distinct colors to each element
    colors[0] = vec4(1.0, 0.0, 0.0, 1.0);  // Red
    colors[1] = vec4(0.0, 1.0, 0.0, 1.0);  // Green
    colors[2] = vec4(0.0, 0.0, 1.0, 1.0);  // Blue
    colors[3] = vec4(1.0, 1.0, 0.0, 1.0);  // Yellow
    colors[4] = vec4(1.0, 0.0, 1.0, 1.0);  // Magenta
    colors[5] = vec4(0.0, 1.0, 1.0, 1.0);  // Cyan

    colourOut = colors[int(chunk) % 6];

    vertexSpeed = vec2(vertexSpeedIn.x, vertexSpeedIn.y);
    gl_Position = vec4(vertexPosition.x / sf, vertexPosition.y / sf, 0.0, 1.0);
}