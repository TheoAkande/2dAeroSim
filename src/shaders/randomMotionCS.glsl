#version 430

layout (local_size_x = 1) in; // sets the number of invocations per work group to 1

layout (binding = 0) buffer inputBuffer { float inVals[]; };
layout (binding = 1) buffer outputBuffer { float outVals[]; };
layout (binding = 2) buffer edgeBuffer { float edgeVals[]; };
layout (binding = 3) buffer chunkBuffer { int chunkVals[]; };
layout (binding = 4) buffer cumChunkSizesBuffer { int cumChunkSizes[]; };
layout (binding = 5) buffer chunkSizesBuffer { int chunkSizes[]; };


uniform float rangeOfMotion;
uniform float vMax;
uniform int numFloats;
uniform float dt;
uniform float xForce;
uniform float yForce;
uniform int numEdges;
uniform int numChunksX;
uniform int numChunksY;
uniform float chunkWidth;
uniform float chunkHeight;
uniform float sf;
uniform float particleProximityThreshold;
uniform float edgeElasticity;

struct Point {
    float x;
    float y;
};

struct Line {
    Point p1;
    Point p2;
};

bool almostEqual(float a, float b) {
    float epsilon = 0.000001;
    return abs(a - b) <= epsilon;
}

float fmax(float a, float b) {
    return a > b ? a : b;
}

float fmin(float a, float b) {
    return a < b ? a : b;
}

// 0 -> collinear
// 1 -> clockwise
// 2 -> counterclockwise
int orientation(Point p, Point q, Point r) {
    float val = (q.y - p.y) * (r.x - q.x) - (q.x - p.x) * (r.y - q.y);
    if (almostEqual(val, 0.0)) return 0;
    return (val > 0) ? 1 : 2;
}

bool on_segment(Point p, Point q, Point r) {
    return q.x <= fmax(p.x, r.x) && q.x >= fmin(p.x, r.x) && q.y <= fmax(p.y, r.y) && q.y >= fmin(p.y, r.y);
}

Point order(float n1, float n2) {
    if (n1 < n2) {
        return Point(n1, n2);
    } else {
        return Point(n2, n1);
    }
}

int checkSign(float n) {
    if (n == 0.0) {
        return 0;
    } else {
        return n < 0.0 ? -1 : 1;
    }
}

// Check if bounding boxes of line and particle intersect
bool boundingIntersect(Line l1, Line l2) {
    Point ordered_1 = order(l1.p1.x, l1.p2.x);
    Point ordered_2 = order(l2.p1.x, l2.p2.x);
    Point ordered_3 = order(l1.p1.y, l1.p2.y);
    Point ordered_4 = order(l2.p1.y, l2.p2.y);

    return ordered_1.x <= ordered_2.y && ordered_1.y >= ordered_2.x && ordered_3.x <= ordered_4.y && ordered_3.y >= ordered_4.x;
}

bool intersect(Line l1, Line l2) {
    if (!boundingIntersect(l1, l2)) {   // easily prune most cases
        return false;
    }
    int o1 = orientation(l1.p1, l1.p2, l2.p1);
    int o2 = orientation(l1.p1, l1.p2, l2.p2);
    int o3 = orientation(l2.p1, l2.p2, l1.p1);
    int o4 = orientation(l2.p1, l2.p2, l1.p2);

    if (o1 != o2 && o3 != o4) {
        return true;
    }

    if (o1 == 0 && on_segment(l1.p1, l2.p1, l1.p2)) return true;
    if (o2 == 0 && on_segment(l1.p1, l2.p2, l1.p2)) return true;
    if (o3 == 0 && on_segment(l2.p1, l1.p1, l2.p2)) return true;
    if (o4 == 0 && on_segment(l2.p1, l1.p2, l2.p2)) return true;

    return false;
}

uint hash(uint x) {
    x = ((x >> 16u) ^ x) * 0x45d9f3bu;
    x = ((x >> 16u) ^ x) * 0x45d9f3bu;
    x = (x >> 16u) ^ x;
    return x;
}

float random(uint seed) {
    return float(hash(seed)) / float(0xffffffffu);
}

float clampedVel(float vel) {
    if (abs(vel) >= vMax) {
        return vMax * (vel > 0.0 ? 1.0 : -1.0);
    } else {
        return vel;
    }
}

void main()
{ 
    uint thisIndex = gl_GlobalInvocationID.x;

    float curX = inVals[thisIndex * numFloats];
    float curY = inVals[thisIndex * numFloats + 1];

    float curVelX = inVals[thisIndex * numFloats + 2];
    float curVelY = inVals[thisIndex * numFloats + 3];

    float newX;
    float newY;
    float newVX;
    float newVY;
    float newAX;
    float newAY;

    // Check if particle collides with any other particles
    float particleForceX = 0.0;
    float particleForceY = 0.0;
    int chunk = int(inVals[thisIndex * numFloats + 7]);
    if (chunk >= 0 && chunk < numChunksX * numChunksY) {
        int particlesInChunk = chunkSizes[chunk];
        int chunkOffset = cumChunkSizes[chunk];
        for (int i = 0; i < particlesInChunk; i++) {
            int otherIndex = chunkVals[chunkOffset + i];
            if (uint(otherIndex) == thisIndex) {
                continue;
            }

            float otherX = inVals[otherIndex * numFloats];
            float otherY = inVals[otherIndex * numFloats + 1];

            if (abs(curX - otherX) < particleProximityThreshold && abs(curY - otherY) < particleProximityThreshold) {
                float otherVelX = inVals[chunkVals[chunkOffset + i] * numFloats + 2];
                float otherVelY = inVals[chunkVals[chunkOffset + i] * numFloats + 3];

                vec2 deltaV = vec2(curVelX - otherVelX, curVelY - otherVelY);
                vec2 deltaP = vec2(curX - otherX, curY - otherY);
                float pSquared = dot(deltaP, deltaP);
                vec2 change = vec2(0, 0);
                if (pSquared != 0) {
                    change = (dot(deltaV, deltaP) / pSquared) * deltaP;
                }
                particleForceX -= change.x;
                particleForceY -= change.y;
            }
        }
        
        curVelX += particleForceX;
        curVelY += particleForceY;

        newX = curX + clampedVel(curVelX) * dt;
        newY = curY + clampedVel(curVelY) * dt;
        newVX = clampedVel(curVelX + inVals[thisIndex * numFloats + 4] * dt);
        newVY = clampedVel(curVelY + inVals[thisIndex * numFloats + 5] * dt);

        uint seed = uint(inVals[thisIndex * numFloats + 6]);
        float rand = (random(seed) - 0.5) * rangeOfMotion * dt;
        newAX = xForce * dt + rand;      
        seed = hash(seed);
        rand = (random(seed) - 0.5) * rangeOfMotion * dt;
        newAY = yForce * dt + rand;      
        seed = hash(seed);
        outVals[thisIndex * numFloats + 6] = seed;  

        Point p3 = Point(curX, curY);
        Point p4 = Point(newX, newY);
        Line path = Line(p3, p4);

        // Check for collisions with edges
        for (int i = 0; i < numEdges; i++) {
            Point p1 = Point(edgeVals[i * 6], edgeVals[i * 6 + 1]);
            Point p2 = Point(edgeVals[i * 6 + 2], edgeVals[i * 6 + 3]);
            Line edge = Line(p1, p2);
            if (intersect(path, edge)) { // Intersect with line
                vec2 edgeVec = vec2(edgeVals[i * 6 + 4], edgeVals[i * 6 + 5]);
                vec2 velVec = vec2(inVals[thisIndex * numFloats + 2], inVals[thisIndex * numFloats + 3]);
                vec2 reflectedVel = reflect(velVec, normalize(edgeVec)) * edgeElasticity;
                newX = curX;
                newY = curY; 
                newVX = reflectedVel.x;
                newVY = reflectedVel.y;
            }
        }

        // Check new chunk position
        int chunkX = int((newX + sf) / chunkWidth);
        int chunkY = int((newY + sf) / chunkHeight);
        float chunkIndex = float(chunkY * numChunksX + chunkX);

        outVals[thisIndex * numFloats] = newX;   
        outVals[thisIndex * numFloats + 1] = newY;   
        outVals[thisIndex * numFloats + 2] = newVX;
        outVals[thisIndex * numFloats + 3] = newVY;
        outVals[thisIndex * numFloats + 4] = newAX;
        outVals[thisIndex * numFloats + 5] = newAY;  
        outVals[thisIndex * numFloats + 7] = chunkIndex; 
    } else {
        outVals[thisIndex * numFloats] = curX;   
        outVals[thisIndex * numFloats + 1] = curY; 
    }                                                                           
}