#version 430

layout (local_size_x = 1) in; // sets the number of invocations per work group to 1

layout (binding = 0) buffer inputBuffer { int inVals[]; };
layout (binding = 2) buffer outputBuffer { int outVals[]; };

uniform int numParticlesX;
uniform float rangeOfMotion;

uint hash(uint x) {
    x = ((x >> 16u) ^ x) * 0x45d9f3bu;
    x = ((x >> 16u) ^ x) * 0x45d9f3bu;
    x = (x >> 16u) ^ x;
    return x;
}

float random(uint seed) {
    return float(hash(seed)) / float(0xffffffffu);
}

void main()
{ 
    uint thisX = gl_GlobalInvocationID.x;
    uint thisY = gl_GlobalInvocationID.y;
    int thisIndex = (thisY * numParticlesX) + thisX;
    uint seed = uint(inVals[thisIndex * 3 + 2]);
    float rand = (random(seed) - 0.5) * rangeOfMotion;
    outVals[thisIndex * 3] = inVals[thisIndex * 3] + rand;

    seed = hash(seed);
    rand = (random(seed) - 0.5) * rangeOfMotion;
    outVals[thisIndex * 3 + 1] = inVals[thisIndex * 3 + 1] + rand;
    seed = hash(seed);
    outVals[thisIndex * 3 + 2] = seed;
}