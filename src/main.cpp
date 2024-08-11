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
#define rangeOfMotion 0.01f

#define numObjects 1

#define numVBOs 2
#define numVAOs 1
#define numCBs 2

// for window
int height = 1080;
int width = 1920;
GLuint vao[numVAOs];
GLuint vbo[numVBOs];
GLuint computeBuffers[numCBs];
GLuint particleRenderingProgram, objectRenderingProgram, computeProgram;
float inBuffer[numParticlesX * numParticlesY * 3];
float outBuffer[numParticlesX * numParticlesY * 3];

struct Particle {
    float x, y;
    float vx, vy;
    float ax, ay;
    GLuint seed;
};

struct Line {
    float x, y;
    float dx, dy;
};

struct Object {
    vector<glm::vec2> *vertices;
    vector<Line> *edges;
    float mass, scale;
    float x, y;
};

Particle particles[numParticlesX * numParticlesY];
Object objects[numObjects];
glm::mat4 objectTransform;

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

void createParticles(void) {
    for (int i = 0; i < numParticlesX; i++) {
        for (int j = 0; j < numParticlesY; j++) {
            particles[i + j * numParticlesX].x = 
                -((float)i / (float)numParticlesX) - (1 / ((float)numParticlesX * 2));
            particles[i + j * numParticlesX].y = 
                ((float)j - ((float)numParticlesY / 2.0f)) * 2.0f / (float)numParticlesY + (1 / ((float)numParticlesY));
            particles[i + j * numParticlesX].vx = 0;
            particles[i + j * numParticlesX].vy = 0;
            particles[i + j * numParticlesX].ax = 0;
            particles[i + j * numParticlesX].ay = 0;
            particles[i + j * numParticlesX].seed = rand();
        }
    }
}

void setupScene(void) {

    vector<float> particlePoints;
    vector<float> objectLines;

    for (int i = 0; i < numParticlesX * numParticlesY; i++) {
        particlePoints.push_back(particles[i].x);
        particlePoints.push_back(particles[i].y);
    }

    for (int i = 0; i < objects[0].vertices->size(); i++) {
        objectLines.push_back(objects[0].vertices->at(i).x);
        objectLines.push_back(objects[0].vertices->at(i).y);
    }

    glGenVertexArrays(numVAOs, vao);
    glBindVertexArray(vao[0]);
    glGenBuffers(numVBOs, vbo);
    
    // Particles VBO
    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
    glBufferData(GL_ARRAY_BUFFER, particlePoints.size() * sizeof(float), &particlePoints[0], GL_STATIC_DRAW);

    // Objects VBO
    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
    glBufferData(GL_ARRAY_BUFFER, objectLines.size() * sizeof(float), &objectLines[0], GL_STATIC_DRAW);
}

void setupComputeBuffers(void) {

    for (int i = 0; i < numParticlesX * numParticlesY; i++) {
        inBuffer[i * 3] = particles[i].x;
        inBuffer[i * 3 + 1] = particles[i].y;
        inBuffer[i * 3 + 2] = particles[i].seed;
    }

    glGenBuffers(numCBs, computeBuffers);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, computeBuffers[0]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, numParticlesX * numParticlesY * 3 * sizeof(float), inBuffer, GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, computeBuffers[1]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, numParticlesX * numParticlesY * 3 * sizeof(float), NULL, GL_STATIC_READ);
}

void display(GLFWwindow *window, double currentTime) {
    glClear(GL_DEPTH_BUFFER_BIT);
    glClear(GL_COLOR_BUFFER_BIT);
    glfwGetFramebufferSize(window, &width, &height);

    // Draw particles
    glUseProgram(particleRenderingProgram);
    glPointSize(3.0f);
    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
    glVertexAttribPointer(0, 2, GL_FLOAT, false, 0, 0);
    glEnableVertexAttribArray(0);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDrawArrays(GL_POINTS, 0, numParticlesX * numParticlesY);

    // Draw object
    glUseProgram(objectRenderingProgram);
    glPointSize(5.0f);
    objectTransform = glm::mat4(1.0f);
    objectTransform *= glm::translate(objectTransform, glm::vec3(objects[0].x, objects[0].y, 0.0f));
    objectTransform *= glm::scale(objectTransform, glm::vec3(objects[0].scale, objects[0].scale, 1.0f));

    glUniformMatrix4fv(glGetUniformLocation(objectRenderingProgram, "model"), 1, GL_FALSE, glm::value_ptr(objectTransform));

    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
    glVertexAttribPointer(0, 2, GL_FLOAT, false, 0, 0);
    glEnableVertexAttribArray(0);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDrawArrays(GL_LINE_STRIP, 0, objects[0].vertices->size());

    glfwSwapBuffers(window);
    glfwPollEvents();
}

void init(void) {
    objects[0] = load2dObject("assets/objects/triangle.2dObj");

    particleRenderingProgram = Utils::createShaderProgram("shaders/particleVert.glsl", "shaders/particleFrag.glsl");
    objectRenderingProgram = Utils::createShaderProgram("shaders/objectVert.glsl", "shaders/objectFrag.glsl");
    computeProgram = Utils::createShaderProgram("shaders/randomMotionCS.glsl");

    createParticles();

    setupScene();
    setupComputeBuffers();
}

void runFrame(GLFWwindow *window, double currentTime) {
    display(window, currentTime);

    glUseProgram(computeProgram);

    GLuint romLoc = glGetUniformLocation(computeProgram, "rangeOfMotion");
    glUniform1f(romLoc, rangeOfMotion);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, computeBuffers[0]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, computeBuffers[1]);
    glDispatchCompute(numParticlesX * numParticlesY, 1, 1); 
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, computeBuffers[1]);
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(outBuffer), outBuffer);

    for (int i = 0; i < numParticlesX * numParticlesY; i++) {
        particles[i].x = outBuffer[i * 3];
        particles[i].y = outBuffer[i * 3 + 1];
        inBuffer[i * 3] = outBuffer[i * 3];
        inBuffer[i * 3 + 1] = outBuffer[i * 3 + 1];
        inBuffer[i * 3 + 2] = outBuffer[i * 3 + 2];
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, computeBuffers[0]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, numParticlesX * numParticlesY * 3 * sizeof(float), inBuffer, GL_STATIC_DRAW);

    setupScene();

}

int main(void) {
    if (!glfwInit()) { 
        exit(EXIT_FAILURE); 
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    GLFWwindow* window = glfwCreateWindow(width, height, "2dAeroSim", NULL, NULL);
    glfwMakeContextCurrent(window);
    if (glewInit() != GLEW_OK) { 
        exit(EXIT_FAILURE); 
    }
    glfwSwapInterval(1);
    init();
    while (!glfwWindowShouldClose(window)) {
        runFrame(window, glfwGetTime());
    }
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}