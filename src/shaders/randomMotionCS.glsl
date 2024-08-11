#version 430

layout (local_size_x = 1) in; // sets the number of invocations per work group to 1

layout (binding = 0) buffer inputBuffer { float inVals[]; };
layout (binding = 1) buffer outputBuffer { float outVals[]; };

uniform float rangeOfMotion;
uniform float vMax;
uniform int numFloats;
uniform float dt;

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
    float xSign = inVals[thisIndex * numFloats + 2] > 0.0 ? 1.0 : -1.0;
    float ySign = inVals[thisIndex * numFloats + 3] > 0.0 ? 1.0 : -1.0;
    outVals[thisIndex * numFloats] = 
        inVals[thisIndex * numFloats] 
        + ((abs(inVals[thisIndex * numFloats + 2]) > vMax ? vMax * xSign : inVals[thisIndex * numFloats + 2])) * dt;                    // x = x + vx (or vMax)
    outVals[thisIndex * numFloats + 1] = inVals[thisIndex * numFloats + 1] 
        + ((abs(inVals[thisIndex * numFloats + 3]) > vMax ? vMax * ySign : inVals[thisIndex * numFloats + 3])) * dt;                    // y = y + vy (or vMax)
    outVals[thisIndex * numFloats + 2] = inVals[thisIndex * numFloats + 2] + inVals[thisIndex * numFloats + 4];     // vx = vx + ax
    outVals[thisIndex * numFloats + 3] = inVals[thisIndex * numFloats + 3] + inVals[thisIndex * numFloats + 5];     // vy = vy + ay
    uint seed = uint(inVals[thisIndex * numFloats + 6]);
    float rand = (random(seed) - 0.5) * rangeOfMotion;
    outVals[thisIndex * numFloats + 4] = inVals[thisIndex * numFloats + 4] + rand;                                  // ax = ax + random
    seed = hash(seed);
    rand = (random(seed) - 0.5) * rangeOfMotion;
    outVals[thisIndex * numFloats + 5] = inVals[thisIndex * numFloats + 5] + rand;                                  // ay = ay + random
    seed = hash(seed);
    outVals[thisIndex * numFloats + 6] = seed;                                                                      // seed = newSeed                                    
}