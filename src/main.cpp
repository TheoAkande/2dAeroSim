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
    vector<Line> *edges;
    float mass, scale;
    float x, y;
};

struct Line {
    float x, y;
    float dx, dy;
};


Particle particles[numParticlesX + numParticlesY];
Object objects[numObjects];

Object load2dObject(const char *filePath) {

    Object newObject;
    vector<glm::vec2> *vertices = new vector<glm::vec2>();
    vector<Line> *edges = new vector<Line>();

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

    for (int i = 0; i < vertices->size(); i++) {
        Line edge;
        edge.x = vertices->at(i).x;
        edge.y = vertices->at(i).y;
        edge.dx = vertices->at((i + 1) % vertices->size()).x - vertices->at(i).x;
        edge.dy = vertices->at((i + 1) % vertices->size()).y - vertices->at(i).y;
        edges->push_back(edge);
    }

    return newObject;
}

void printObject(Object object) {
    cout << "Mass: " << object.mass << endl;
    cout << "Scale: " << object.scale << endl;
    cout << "Position: (" << object.x << ", " << object.y << ")" << endl;
    for (int i = 0; i < object.vertices->size(); i++) {
        cout << "Vertex " << i << ": " << object.vertices->at(i).x << ", " << object.vertices->at(i).y << endl;
    }
}

void init(void) {
    objects[0] = load2dObject("assets/objects/triangle.2dObj");
}

int main(void) {

    init();

    printObject(objects[0]);

    exit(EXIT_SUCCESS);

}