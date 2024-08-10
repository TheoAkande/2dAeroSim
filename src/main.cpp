#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <string>
#include <iostream>
#include <fstream>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <SOIL2/soil2.h>

#include "Utils.h"

using namespace std;

#define numParticlesX 50
#define numParticlesY 50
#define particleMass 1.0f

#define numObjects 1

struct Particle {
    float x, y;
    float vx, vy;
    float ax, ay;
};

struct Object {
    vector<glm::vec2> *vertices;
    float mass, scale;
    float x, y;
};

Particle particles[numParticlesX + numParticlesY];
Object objects[numObjects];

Object load2dObject(const char *filePath) {

    Object newObject;
    vector<glm::vec2> *vertices = new vector<glm::vec2>();

    newObject.vertices = vertices;
    newObject.x = 0.0f;
    newObject.y = 0.0f;
    newObject.mass = 1.0f;
    newObject.scale = 1.0f;

    ifstream fileStream(filePath, ios::in);
    string line = "";
    while (!fileStream.eof()) {
        getline(fileStream, line);
        if (line.c_str()[0] == 'v') {
            glm::vec2 vertex;
            sscanf(line.c_str(), "v %f %f", &vertex.x, &vertex.y);
            vertices->push_back(vertex);
        } else if (line.c_str()[0] == 'm') {
            sscanf(line.c_str(), "m %f", &newObject.mass);
        } else if (line.c_str()[0] == 's') {
            sscanf(line.c_str(), "s %f", &newObject.scale);
        }
    }
    fileStream.close();
    return newObject;
}

void init(void) {

    objects[0] = load2dObject("assets/objects/triangle.2dObj");

}

int main(void) {

    init();

    for (int i = 0; i < objects[0].vertices->size(); i++) {
        cout << "Vertex " << i << ": " << objects[0].vertices->at(i).x << ", " << objects[0].vertices->at(i).y << endl;
    }
    cout << "Mass: " << objects[0].mass << endl;
    cout << "Scale: " << objects[0].scale << endl;

    exit(EXIT_SUCCESS);

}