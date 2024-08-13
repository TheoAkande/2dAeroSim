#version 430

layout (local_size_x = 1) in; // sets the number of invocations per work group to 1

layout (binding = 0) buffer inputBuffer { float inVals[]; };
layout (binding = 1) buffer outputBuffer { float outVals[]; };
layout (binding = 2) buffer edgeBuffer { float edgeVals[]; };

uniform float rangeOfMotion;
uniform float vMax;
uniform int numFloats;
uniform float dt;
uniform float xForce;
uniform float yForce;
uniform int numEdges;

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

int orientation(Point p, Point q, Point r) {
    float val = (q.y - p.y) * (r.x - q.x) - (q.x - p.x) * (r.y - q.y);
    if (almostEqual(val, 0.0)) return 0;
    return (val > 0) ? 1 : 2;
}

Point order(float n1, float n2) {
    if (n1 < n2) {
        return Point(n1, n2);
    } else {
        return Point(n2, n1);
    }
}

// Pass point path then object line
Point intersectPoint(Line l1, Line l2) {

    float dx1 = l1.p2.x - l1.p1.x;
    float dx2 = l2.p2.x - l2.p1.x;

    if (dx1 == 0 && dx2 == 0) {
        Point ordered_1 = order(l1.p1.y, l1.p2.y);
        Point ordered_2 = order(l2.p1.y, l2.p2.y);

        if (ordered_1.y < ordered_2.x || ordered_1.x > ordered_2.y) {   // not intersect
            return Point(l1.p1.x + 5.0, 0.0);
        } else {
            return l1.p2;
        }
    }

    if (dx1 == 0) {
        Point ordered_2 = order(l2.p1.y, l2.p2.y);

        if (ordered_2.x <= l1.p1.x && ordered_2.y >= l1.p1.x) {
            return Point(l1.p1.x, l1.p2.y);
        } else {
            return l2.p1;
        }
    }

    if (dx2 == 0) {
        Point ordered_1 = order(l1.p1.y, l1.p2.y);

        if (ordered_1.x <= l2.p1.x && ordered_1.y >= l2.p1.x) {
            return Point(l2.p1.x, l1.p2.y);
        } else {
            return l2.p1;
        }
    }

    float m1 = (l1.p2.y - l1.p1.y) / dx1;
    float m2 = (l2.p2.y - l2.p1.y) / dx2;

    if (almostEqual(abs(m1), abs(m2))) {    // if collinear
        Point ordered_1 = order(l1.p1.x, l1.p2.x);
        Point ordered_2 = order(l2.p1.x, l2.p2.x);

        if (ordered_1.x >= ordered_2.y || ordered_1.y <= ordered_2.x) { // Not intersecting
            return l2.p1;
        } else {
            if (ordered_1.x >= ordered_2.x) {
                if (ordered_1.x == l1.p1.x) {
                    return l1.p1;
                } else {
                    return l1.p2;
                }
            } else if (ordered_1.y <= ordered_2.y) {
                if (ordered_1.x == l1.p1.x) {
                    return l1.p1;
                } else {
                    return l1.p2;
                }
            } else {
                return Point((l2.p1.x + l2.p2.x) / 2, (l2.p1.y + l2.p2.y) / 2);
            }
        }
    } else {
        float x1 = l1.p1.x;
        float y1 = l1.p1.y;
        float x2 = l2.p1.x;
        float y2 = l2.p1.y;

        // m1 (x - x1) + y1 = m2 (x - x2) + y2
        // m1 . x - m2 . x = m1 . x1 - m2 . x2 - y1 + y2
        float x = (m1 * x1 - m2 * x2 - y1 + y2) / (m1 - m2);
        float y = m1 * (x - x1) + y1;

        return Point(x, y);
    }
}


bool intersect(Line l1, Line l2) {
    int o1 = orientation(l1.p1, l1.p2, l2.p1);
    int o2 = orientation(l1.p1, l1.p2, l2.p2);
    int o3 = orientation(l2.p1, l2.p2, l1.p1);
    int o4 = orientation(l2.p1, l2.p2, l1.p2);
    if (!(almostEqual(o1, o2)) && !(almostEqual(o3, o4))) return true;
    if (almostEqual(o1, 0) && almostEqual(o2, 0) && almostEqual(o3, 0) && almostEqual(o4, 0)) {
        if (l1.p1.x == l1.p2.x) {
            if (l1.p1.y > l1.p2.y) {
                Point temp = l1.p1;
                l1.p1 = l1.p2;
                l1.p2 = temp;
            }
            if (l2.p1.y > l2.p2.y) {
                Point temp = l2.p1;
                l2.p1 = l2.p2;
                l2.p2 = temp;
            }
            return l1.p1.y <= l2.p2.y && l1.p2.y >= l2.p1.y;
        } else {
            if (l1.p1.x > l1.p2.x) {
                Point temp = l1.p1;
                l1.p1 = l1.p2;
                l1.p2 = temp;
            }
            if (l2.p1.x > l2.p2.x) {
                Point temp = l2.p1;
                l2.p1 = l2.p2;
                l2.p2 = temp;
            }
            return l1.p1.x <= l2.p2.x && l1.p2.x >= l2.p1.x;
        }
    }
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

void main()
{ 
    uint thisIndex = gl_GlobalInvocationID.x;

    float curX = inVals[thisIndex * numFloats];
    float curY = inVals[thisIndex * numFloats + 1];
    float newX;
    float newY;
    float newVX;
    float newVY;
    float newAX;
    float newAY;

    if (abs(inVals[thisIndex * numFloats + 2]) >= vMax) {
        float xSign = inVals[thisIndex * numFloats + 2] > 0.0 ? 1.0 : -1.0;
        newX = inVals[thisIndex * numFloats] + vMax * xSign * dt;
        if (abs(inVals[thisIndex * numFloats + 2] + inVals[thisIndex * numFloats + 4]) >= abs(inVals[thisIndex * numFloats + 2])) {
            newVX = vMax * xSign;
        } else {
            newVX = inVals[thisIndex * numFloats + 2] + inVals[thisIndex * numFloats + 4] * dt;
        }
    } else {
        newX = inVals[thisIndex * numFloats] + inVals[thisIndex * numFloats + 2] * dt;
        newVX = inVals[thisIndex * numFloats + 2] + inVals[thisIndex * numFloats + 4] * dt;
    }
    if (abs(inVals[thisIndex * numFloats + 3]) >= vMax) {
        float ySign = inVals[thisIndex * numFloats + 3] > 0.0 ? 1.0 : -1.0;
        newY = inVals[thisIndex * numFloats + 1] + vMax * ySign * dt;
        if (abs(inVals[thisIndex * numFloats + 3] + inVals[thisIndex * numFloats + 5]) >= abs(inVals[thisIndex * numFloats + 3])) {
            newVY = vMax * ySign;
        } else {
            newVY = inVals[thisIndex * numFloats + 3] + inVals[thisIndex * numFloats + 5] * dt;
        }
    } else {
        newY = inVals[thisIndex * numFloats + 1] + inVals[thisIndex * numFloats + 3] * dt;
        newVY = inVals[thisIndex * numFloats + 3] + inVals[thisIndex * numFloats + 5] * dt;
    }
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
    // for (int i = 0; i < numEdges; i++) {
    //     Point p1 = Point(edgeVals[i * 6], edgeVals[i * 6 + 1]);
    //     Point p2 = Point(edgeVals[i * 6 + 2], edgeVals[i * 6 + 3]);
    //     Line edge = Line(p1, p2);
    //     Point ip = intersectPoint(path, edge);
    //     Point orderedX = order(curX, newX);
    //     if (orderedX.x <= ip.x && orderedX.y >= ip.x) { // Intersect with line
    //         vec2 edgeVec = vec2(edgeVals[i * 6 + 4], edgeVals[i * 6 + 5]);
    //         vec2 velVec = vec2(inVals[thisIndex * numFloats + 2], inVals[thisIndex * numFloats + 3]);
    //         vec2 reflectedVel = reflect(velVec, normalize(edgeVec));
    //         vec2 accelVec = vec2(inVals[thisIndex * numFloats + 4], inVals[thisIndex * numFloats + 5]);
    //         vec2 reflectedAccel = reflect(accelVec, normalize(edgeVec));

    //         newX = curX + reflectedVel.x * dt;
    //         newY = curY + reflectedVel.y * dt;
    //         newVX = reflectedVel.x;
    //         newVY = reflectedVel.y;
    //         newAX = reflectedAccel.x;
    //         newAY = reflectedAccel.y;
    //     }
    // }

    outVals[thisIndex * numFloats] = newX;   
    outVals[thisIndex * numFloats + 1] = newY;   
    outVals[thisIndex * numFloats + 2] = newVX;
    outVals[thisIndex * numFloats + 3] = newVY;
    outVals[thisIndex * numFloats + 4] = newAX;
    outVals[thisIndex * numFloats + 5] = newAY;                                                                                   
}