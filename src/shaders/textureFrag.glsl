#version 430

in vec2 tc;

out vec4 colour;

layout (binding = 0) uniform sampler2D samp;

void main(void)
{ 
    colour = texture(samp, tc);
}