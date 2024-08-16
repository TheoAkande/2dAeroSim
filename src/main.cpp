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

#define numParticlesX 100
#define numParticlesY 100
#define particleMass 1.0f
#define rangeOfMotion 500.0f
#define vMax 500.0f
#define totalVMaxSquare vMax
#define numParticleFloats 8
#define force 10000.0f

#define scaleFactor 1080.0f
#define numChunksX 10
#define numChunksY 10

#define numObjects 1

#define numVBOs 2
#define numVAOs 1
#define numCBs 3

double pastTime = 0.0;
double deltaTime = 0.0;

// for window
int height = 1080;
int width = 1920;
GLuint vao[numVAOs];
GLuint vbo[numVBOs];

// for compute shader
GLuint  floatNumLoc, romLoc, vMaxLoc, dtLoc, xForceLoc, yForceLoc, numEdgesLoc, 
        sfLoc, tvmsLoc, chunkWdithLoc, chunkHeightLoc, numChunksXLoc, numChunksYLoc;
GLuint computeBuffers[numCBs];
GLuint particleRenderingProgram, objectRenderingProgram, computeProgram;
float *curInBuffer;
float *curOutBuffer;
float buffer1[numParticlesX * numParticlesY * numParticleFloats];
float buffer2[numParticlesX * numParticlesY * numParticleFloats];
float xForce, yForce = 0.0f;
float adjustedChunkWidth, adjustedChunkHeight;

struct Particle {
    float x, y;
    float vx, vy;
    float ax, ay;
    GLuint seed;
    int chunk;
};

struct Line {
    float x1, y1;
    float x2, y2;
    float nx, ny;
};

struct Object {
    vector<glm::vec2> *vertices;
    vector<Line> *edges;
    float mass, scale;
    float x, y;
    int numEdges;
};

struct Chunks {
    vector<int> *chunks[numChunksX * numChunksY];
    int chunkSizes[numChunksX * numChunksY];
    float chunkWidth, chunkHeight;
};

Particle particles[numParticlesX * numParticlesY];
Object objects[numObjects];
Chunks chunks = Chunks();

// Create the chunk indicators that will be used to determine which particles are in which chunks
void setupChunks(void) {
    chunks.chunkWidth = (float)width / (float)numChunksX;
    chunks.chunkHeight = (float)height / (float)numChunksY;
    adjustedChunkWidth = chunks.chunkWidth * scaleFactor;
    adjustedChunkHeight = chunks.chunkHeight * scaleFactor;
    for (int i = 0; i < numChunksX; i++) {
        for (int j = 0; j < numChunksY; j++) {
            chunks.chunks[i + j * numChunksX] = new vector<int>();
            chunks.chunkSizes[i + j * numChunksX] = 0;
        }
    }
}

Object load2dObject(const char *filePath) {

    Object newObject;
    vector<glm::vec2> *vertices = new vector<glm::vec2>();
    vector<Line> *edges = new vector<Line>();

    newObject.vertices = vertices;
    newObject.x = 0.0f;
    newObject.y = 0.0f;
    newObject.mass = 1.0f;
    newObject.scale = 1.0f;

    int nv = 0;

    ifstream fileStream(filePath, ios::in);
    string line = "";
    while (!fileStream.eof()) {
        getline(fileStream, line);
        if (line.c_str()[0] == 'v') {
            glm::vec2 vertex;
            sscanf(line.c_str(), "v %f %f", &vertex.x, &vertex.y);
            vertices->push_back(glm::vec2(vertex.x * scaleFactor, vertex.y * scaleFactor));
            nv++;
        } else if (line.c_str()[0] == 'm') {
            sscanf(line.c_str(), "m %f", &newObject.mass);
        } else if (line.c_str()[0] == 's') {
            sscanf(line.c_str(), "s %f", &newObject.scale);
        }
    }
    fileStream.close();

    for (int i = 0; i < vertices->size() - 1; i++) {
        Line edge;
        edge.x1 = vertices->at(i).x;
        edge.y1 = vertices->at(i).y;
        edge.x2 = vertices->at((i + 1)).x;
        edge.y2 = vertices->at((i + 1)).y;
        edge.nx = edge.y1 - edge.y2;
        edge.ny = edge.x2 - edge.x1;
        edges->push_back(edge);
    }

    newObject.numEdges = nv - 1;
    newObject.edges = edges;

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
    setupChunks();

    for (int i = 0; i < numParticlesX; i++) {
        for (int j = 0; j < numParticlesY; j++) {
            particles[i + j * numParticlesX].x = 
                (-((float)i / (float)numParticlesX) - (1 / ((float)numParticlesX * 2))) * scaleFactor;
            particles[i + j * numParticlesX].y = 
                (((float)j - ((float)numParticlesY / 2.0f)) * 2.0f / (float)numParticlesY + (1 / ((float)numParticlesY))) * scaleFactor;
            particles[i + j * numParticlesX].vx = 0;
            particles[i + j * numParticlesX].vy = 0;
            particles[i + j * numParticlesX].ax = 0;
            particles[i + j * numParticlesX].ay = 0;
            particles[i + j * numParticlesX].seed = rand();
            particles[i + j * numParticlesX].chunk = 
                (int)(particles[i + j * numParticlesX].x / chunks.chunkWidth) + 
                (int)(particles[i + j * numParticlesX].y / chunks.chunkHeight) * numChunksX;
        }
    }
}

void setupScene(void) {
    vector<float> objectLines = vector<float>();

    for (int i = 0; i < numParticlesX * numParticlesY; i++) {
        curInBuffer[i * numParticleFloats] = particles[i].x;
        curInBuffer[i * numParticleFloats + 1] = particles[i].y;
        curInBuffer[i * numParticleFloats + 2] = particles[i].vx;
        curInBuffer[i * numParticleFloats + 3] = particles[i].vy;
        curInBuffer[i * numParticleFloats + 4] = particles[i].ax;
        curInBuffer[i * numParticleFloats + 5] = particles[i].ay;
        curInBuffer[i * numParticleFloats + 6] = particles[i].seed;
        curInBuffer[i * numParticleFloats + 7] = static_cast<float>(particles[i].chunk);
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
    glBufferData(GL_ARRAY_BUFFER, sizeof(buffer1), &curInBuffer[0], GL_STATIC_DRAW);

    // Objects VBO
    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
    glBufferData(GL_ARRAY_BUFFER, objectLines.size() * sizeof(float), &objectLines[0], GL_STATIC_DRAW);
}

void bindComputeBuffers(void) {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, computeBuffers[0]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, numParticlesX * numParticlesY * numParticleFloats * sizeof(float), curInBuffer, GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, computeBuffers[1]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, numParticlesX * numParticlesY * numParticleFloats * sizeof(float), NULL, GL_STATIC_READ);
}

void setupComputeBuffers(void) {
    for (int i = 0; i < numParticlesX * numParticlesY; i++) {
        curInBuffer[i * numParticleFloats] = particles[i].x;
        curInBuffer[i * numParticleFloats + 1] = particles[i].y;
        curInBuffer[i * numParticleFloats + 2] = particles[i].vx;
        curInBuffer[i * numParticleFloats + 3] = particles[i].vy;
        curInBuffer[i * numParticleFloats + 4] = particles[i].ax;
        curInBuffer[i * numParticleFloats + 5] = particles[i].ay;
        curInBuffer[i * numParticleFloats + 6] = particles[i].seed;
        curInBuffer[i * numParticleFloats + 7] = static_cast<float>(particles[i].chunk);
    }

    glGenBuffers(numCBs, computeBuffers);
    bindComputeBuffers();

    // Doesn't need to be rebound each time (yet at least)
    vector<float> objectEdges;
    for (int i = 0; i < objects[0].numEdges; i++) {
        objectEdges.push_back(objects[0].edges->at(i).x1);
        objectEdges.push_back(objects[0].edges->at(i).y1);
        objectEdges.push_back(objects[0].edges->at(i).x2);
        objectEdges.push_back(objects[0].edges->at(i).y2);
        objectEdges.push_back(objects[0].edges->at(i).nx);
        objectEdges.push_back(objects[0].edges->at(i).ny);
    }
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, computeBuffers[2]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, objects[0].numEdges * 6 * sizeof(float), objectEdges.data(), GL_STATIC_DRAW);
}

void display(GLFWwindow *window) {
    glClear(GL_DEPTH_BUFFER_BIT);
    glClear(GL_COLOR_BUFFER_BIT);
    glfwGetFramebufferSize(window, &width, &height);

    // Draw particles
    glUseProgram(particleRenderingProgram);
    glPointSize(3.0f);
    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(buffer1), &curInBuffer[0], GL_STATIC_DRAW);

    sfLoc = glGetUniformLocation(particleRenderingProgram, "sf");
    glUniform1f(sfLoc, scaleFactor);

    tvmsLoc = glGetUniformLocation(particleRenderingProgram, "totalVMax");
    glUniform1f(tvmsLoc, vMax);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, numParticleFloats * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, numParticleFloats * sizeof(float), (void *)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDrawArrays(GL_POINTS, 0, numParticlesX * numParticlesY);

    // Draw object
    glUseProgram(objectRenderingProgram);
    glPointSize(5.0f);
    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);

    sfLoc = glGetUniformLocation(objectRenderingProgram, "sf");
    glUniform1f(sfLoc, scaleFactor);

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

    curInBuffer = &buffer1[0];
    curOutBuffer = &buffer2[0];
    setupScene();
    setupComputeBuffers();
}

void runFrame(GLFWwindow *window, double currentTime) {
    deltaTime = currentTime - pastTime;
    pastTime = currentTime;

    display(window);

    glUseProgram(computeProgram);

    romLoc = glGetUniformLocation(computeProgram, "rangeOfMotion");
    glUniform1f(romLoc, rangeOfMotion);
    floatNumLoc = glGetUniformLocation(computeProgram, "numFloats");
    glUniform1i(floatNumLoc, numParticleFloats);
    vMaxLoc = glGetUniformLocation(computeProgram, "vMax");
    glUniform1f(vMaxLoc, vMax);
    dtLoc = glGetUniformLocation(computeProgram, "dt");
    glUniform1f(dtLoc, (float)deltaTime);
    xForceLoc = glGetUniformLocation(computeProgram, "xForce");
    glUniform1f(xForceLoc, xForce);
    yForceLoc = glGetUniformLocation(computeProgram, "yForce");
    glUniform1f(yForceLoc, yForce);
    numEdgesLoc = glGetUniformLocation(computeProgram, "numEdges");
    glUniform1i(numEdgesLoc, objects[0].numEdges);
    chunkWdithLoc = glGetUniformLocation(computeProgram, "chunkWidth");
    glUniform1f(chunkWdithLoc, adjustedChunkWidth);
    chunkHeightLoc = glGetUniformLocation(computeProgram, "chunkHeight");
    glUniform1f(chunkHeightLoc, adjustedChunkHeight);
    numChunksXLoc = glGetUniformLocation(computeProgram, "numChunksX");
    glUniform1i(numChunksXLoc, numChunksX);
    numChunksYLoc = glGetUniformLocation(computeProgram, "numChunksY");
    glUniform1i(numChunksYLoc, numChunksY);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, computeBuffers[0]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, computeBuffers[1]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, computeBuffers[2]);
    glDispatchCompute(numParticlesX * numParticlesY, 1, 1); 
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, computeBuffers[1]);
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(buffer1), curOutBuffer);

    bindComputeBuffers();
    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(buffer1), &curInBuffer[0], GL_STATIC_DRAW);

    float *temp = curOutBuffer;
    curOutBuffer = curInBuffer;
    curInBuffer = temp;

    xForce = 0.0f;
    yForce = 0.0f;
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        xForce = -force;
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        xForce = force;
    }
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        yForce = force;
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        yForce = -force;
    }
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