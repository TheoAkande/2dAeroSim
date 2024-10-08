#version 430

layout (location = 0) in vec2 pos;
layout (location = 1) in vec2 texCoord;

out vec2 tc;

layout (binding = 0) uniform sampler2D samp;

void main(void)
{	 
    gl_Position = vec4(pos, 0.0, 1.0);
    tc = texCoord;
}