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

vector<glm::vec2> load2dObject(const char *filePath) {
    vector<glm::vec2> vertices;
    ifstream fileStream(filePath, ios::in);
    string line = "";
    while (!fileStream.eof()) {
        getline(fileStream, line);
        if (line.c_str()[0] == 'v') {
            glm::vec2 vertex;
            sscanf(line.c_str(), "v %f %f", &vertex.x, &vertex.y);
            vertices.push_back(vertex);
        }
    }
    fileStream.close();
    return vertices;
}

int main(void) {

    vector<glm::vec2> vertices = load2dObject("assets/objects/triangle.2dObj");
    for (int i = 0; i < vertices.size(); i++) {
        cout << "Vertex " << i << ": " << vertices[i].x << ", " << vertices[i].y << endl;
    }

    exit(EXIT_SUCCESS);

}