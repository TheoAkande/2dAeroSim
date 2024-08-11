#version 430

layout (local_size_x = 1) in; // sets the number of invocations per work group to 1

layout (binding = 0) buffer inputBuffer { float inVals[]; };
layout (binding = 1) buffer outputBuffer { float outVals[]; };

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
    uint thisIndex = gl_GlobalInvocationID.x;
    uint seed = uint(inVals[thisIndex * 7 + 6]);
    float rand = (random(seed) - 0.5) * rangeOfMotion;
    outVals[thisIndex * 7] = inVals[thisIndex * 7] + rand;
    seed = hash(seed);
    rand = (random(seed) - 0.5) * rangeOfMotion;
    outVals[thisIndex * 7 + 1] = inVals[thisIndex * 7 + 1] + rand;
    seed = hash(seed);
    outVals[thisIndex * 7 + 2] = inVals[thisIndex * 7 + 2];
    outVals[thisIndex * 7 + 3] = inVals[thisIndex * 7 + 3];
    outVals[thisIndex * 7 + 4] = inVals[thisIndex * 7 + 4];
    outVals[thisIndex * 7 + 5] = inVals[thisIndex * 7 + 5];
    outVals[thisIndex * 7 + 6] = seed;
}