#version 430

layout (local_size_x = 1) in; // sets the number of invocations per work group to 1

layout (binding = 0) buffer inputBuffer { float inVals[]; };
layout (binding = 1) buffer outputBuffer { float outVals[]; };

uniform float rangeOfMotion;
uniform float vMax;
uniform int numFloats;
uniform float dt;
uniform float xForce;
uniform float yForce;

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
    if (abs(inVals[thisIndex * numFloats + 2]) >= vMax) {
        float xSign = inVals[thisIndex * numFloats + 2] > 0.0 ? 1.0 : -1.0;
        outVals[thisIndex * numFloats] = inVals[thisIndex * numFloats] + vMax * xSign * dt;
        if (abs(inVals[thisIndex * numFloats + 2] + inVals[thisIndex * numFloats + 4]) >= abs(inVals[thisIndex * numFloats + 2])) {
            outVals[thisIndex * numFloats + 2] = vMax * xSign;
        } else {
            outVals[thisIndex * numFloats + 2] = inVals[thisIndex * numFloats + 2] + inVals[thisIndex * numFloats + 4] * dt;
        }
    } else {
        outVals[thisIndex * numFloats] = inVals[thisIndex * numFloats] + inVals[thisIndex * numFloats + 2] * dt;
        outVals[thisIndex * numFloats + 2] = inVals[thisIndex * numFloats + 2] + inVals[thisIndex * numFloats + 4] * dt;
    }
    if (abs(inVals[thisIndex * numFloats + 3]) >= vMax) {
        float ySign = inVals[thisIndex * numFloats + 3] > 0.0 ? 1.0 : -1.0;
        outVals[thisIndex * numFloats + 1] = inVals[thisIndex * numFloats + 1] + vMax * ySign * dt;
        if (abs(inVals[thisIndex * numFloats + 3] + inVals[thisIndex * numFloats + 5]) >= abs(inVals[thisIndex * numFloats + 3])) {
            outVals[thisIndex * numFloats + 3] = vMax * ySign;
        } else {
            outVals[thisIndex * numFloats + 3] = inVals[thisIndex * numFloats + 3] + inVals[thisIndex * numFloats + 5] * dt;
        }
    } else {
        outVals[thisIndex * numFloats + 1] = inVals[thisIndex * numFloats + 1] + inVals[thisIndex * numFloats + 3] * dt;
        outVals[thisIndex * numFloats + 3] = inVals[thisIndex * numFloats + 3] + inVals[thisIndex * numFloats + 5] * dt;
    }

    uint seed = uint(inVals[thisIndex * numFloats + 6]);
    float rand = (random(seed) - 0.5) * rangeOfMotion * dt;
    outVals[thisIndex * numFloats + 4] = inVals[thisIndex * numFloats + 4] + xForce * dt + rand;      
    seed = hash(seed);
    rand = (random(seed) - 0.5) * rangeOfMotion * dt;
    outVals[thisIndex * numFloats + 5] = inVals[thisIndex * numFloats + 5] + yForce * dt + rand;      
    seed = hash(seed);
    outVals[thisIndex * numFloats + 6] = seed;                                                                                           
}