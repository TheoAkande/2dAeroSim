#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <string>
#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <SOIL2/soil2.h>
#include <cstdlib>

#include "Utils.h"

using namespace std;

#define benchmark false

#define numParticlesX 128
#define numParticlesY 128
#define particleMass 1.0f
#define rangeOfMotion 300.0f
#define vMax 5000.0f
#define colourVMax 300.0f
#define numParticleFloats 8
#define numEdgeFloats 7
#define force 10000.0f
#define edgeElasticity 0.5f

#define scaleFactor 1080.0f
#define numChunksX 32
#define numChunksY 32
#define ppt 1.5f

#define numVBOs 2
#define numVAOs 1
#define numCBs 6
#define workGroupSize 64

#define numObjects 3
const char *assets[] = {"assets/objects/inverted.2dObj", "assets/objects/box.2dObj", "assets/objects/triangle.2dObj"};

double pastTime = 0.0;
double deltaTime = 0.0;

int numParticles = numParticlesX * numParticlesY;

// for window
int height = 1000;
int width = 1000;
GLuint vao[numVAOs];
GLuint vbo[numVBOs];

// for compute shader
GLuint  floatNumLoc, romLoc, vMaxLoc, dtLoc, xForceLoc, yForceLoc, numEdgesLoc, 
        sfLoc, tvmsLoc, chunkWdithLoc, chunkHeightLoc, numChunksXLoc, numChunksYLoc,
        pptLoc, pfLoc, eeLoc;
GLuint computeBuffers[numCBs];
GLuint particleRenderingProgram, objectRenderingProgram, computeProgram;
float *curInBuffer;
float *curOutBuffer;
float buffer1[numParticlesX * numParticlesY * numParticleFloats];
float buffer2[numParticlesX * numParticlesY * numParticleFloats];
float xForce, yForce = 0.0f;

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
    float elasticity;
};

struct Object {
    vector<glm::vec2> *vertices;
    vector<Line> *edges;
    float mass, scale;
    float x, y;
    float elasticity;
    int numEdges;
};

struct Chunks {
    int chunks[numParticlesX * numParticlesY];
    int cumChunkSizes[numChunksX * numChunksY];
    int chunkSizes[numChunksX * numChunksY];
    int chunksCounter[numChunksX * numChunksY];
    float chunkWidth, chunkHeight;
};

Particle particles[numParticlesX * numParticlesY];
Object objects[numObjects];
Chunks chunks = Chunks();
int objectsOffset[numObjects];
int numEdgesTotal = 0;
int objectCounter = 0;

// Create the chunk indicators that will be used to determine which particles are in which chunks
void setupChunks(void) {
    chunks.chunkWidth = (float)(2 * scaleFactor) / (float)numChunksX;
    chunks.chunkHeight = (float)(2 * scaleFactor) / (float)numChunksY;
}

void updateChunkData(float particle_buffer[]) {
    for (int i = 0; i < numChunksX * numChunksY; i++) {
        chunks.chunkSizes[i] = 0;
        chunks.chunksCounter[i] = 0;
    }
    for (int i = 0; i < numParticlesX * numParticlesY; i++) {
        int chunk = particle_buffer[i * numParticleFloats + 7];
        if (chunk >= 0 && chunk < numChunksX * numChunksY) {
            chunks.chunkSizes[chunk]++;
        }
    }
    chunks.cumChunkSizes[0] = 0;
    for (int i = 1; i < numChunksX * numChunksY; i++) {
        chunks.cumChunkSizes[i] = chunks.chunkSizes[i - 1] + chunks.cumChunkSizes[i - 1];
    }
    for (int i = 0; i < numParticlesX * numParticlesY; i++) {
        int chunk = particle_buffer[i * numParticleFloats + 7];
        if (chunk >= 0 && chunk < numChunksX * numChunksY) {
            chunks.chunks[chunks.cumChunkSizes[chunk] + chunks.chunksCounter[chunk]] = i;
            chunks.chunksCounter[chunk]++;
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
    newObject.elasticity = edgeElasticity;

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
        } else if (line.c_str()[0] == 'e') {
            sscanf(line.c_str(), "e %f", &newObject.elasticity, &newObject.elasticity);
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
        edge.elasticity = newObject.elasticity;
        edges->push_back(edge);
    }

    newObject.numEdges = nv - 1;
    newObject.edges = edges;

    if (objectCounter == 0) {
        objectsOffset[0] = 0;
    } else {
        objectsOffset[objectCounter] = objectsOffset[objectCounter - 1] + objects[objectCounter - 1].vertices->size();
    }
    objectCounter++;

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

    // we want to add particles in grids of workGroup sizes for better spatial locality
    // workgroup sizes should be 64 or 32
    int groupWidthX = (workGroupSize == 64) ? 8 : 4;
    int groupWidthY = workGroupSize / groupWidthX;
    int numGroups = numParticles / workGroupSize;

    for (int i = 0; i < numGroups; i++) {

        int groupBaseX = (i % (numParticlesX / groupWidthX)) * groupWidthX;
        int groupBaseY = (i / (numParticlesX / groupWidthX)) * groupWidthY;

        for (int j = 0; j < workGroupSize; j++) {
            int x = groupBaseX + (j % groupWidthX);
            int y = groupBaseY + (j / groupWidthX);

            particles[x + y * numParticlesX].x = 
                (-((float)x / (float)numParticlesX) - (1 / ((float)numParticlesX * 2))) * scaleFactor;
            particles[x + y * numParticlesX].y = 
                (((float)y - ((float)numParticlesY / 2.0f)) * 2.0f / (float)numParticlesY + (1 / ((float)numParticlesY))) * scaleFactor;
            particles[x + y * numParticlesX].vx = 0;
            particles[x + y * numParticlesX].vy = 0;
            particles[x + y * numParticlesX].ax = 0;
            particles[x + y * numParticlesX].ay = 0;
            particles[x + y * numParticlesX].seed = rand();
            particles[x + y * numParticlesX].chunk = 
                (int)((particles[x + y * numParticlesX].x + scaleFactor) / chunks.chunkWidth) + 
                (int)((particles[x + y * numParticlesX].y + scaleFactor) / chunks.chunkHeight) * numChunksX;

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

    for (int j = 0; j < numObjects; j++) {
        for (int i = 0; i < objects[j].vertices->size(); i++) {
            objectLines.push_back(objects[j].vertices->at(i).x);
            objectLines.push_back(objects[j].vertices->at(i).y);
        }
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
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, computeBuffers[3]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, numParticlesX * numParticlesY * sizeof(int), &chunks.chunks[0], GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, computeBuffers[4]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, numChunksX * numChunksY * sizeof(int), &chunks.cumChunkSizes[0], GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, computeBuffers[5]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, numChunksX * numChunksY * sizeof(int), &chunks.chunkSizes[0], GL_STATIC_DRAW);

    
    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(buffer1), curInBuffer, GL_STATIC_DRAW);
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
    for (int j = 0; j < numObjects; j++) {
        for (int i = 0; i < objects[j].numEdges; i++) {
            objectEdges.push_back(objects[j].edges->at(i).x1);
            objectEdges.push_back(objects[j].edges->at(i).y1);
            objectEdges.push_back(objects[j].edges->at(i).x2);
            objectEdges.push_back(objects[j].edges->at(i).y2);
            objectEdges.push_back(objects[j].edges->at(i).nx);
            objectEdges.push_back(objects[j].edges->at(i).ny);
            objectEdges.push_back(objects[j].edges->at(i).elasticity);
        }
        numEdgesTotal += objects[j].numEdges;
    }
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, computeBuffers[2]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, numEdgesTotal * numEdgeFloats * sizeof(float), objectEdges.data(), GL_STATIC_DRAW);
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
    glUniform1f(tvmsLoc, colourVMax);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, numParticleFloats * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, numParticleFloats * sizeof(float), (void *)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDrawArrays(GL_POINTS, 0, numParticlesX * numParticlesY);

    // Draw object
    glUseProgram(objectRenderingProgram);
    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);

    sfLoc = glGetUniformLocation(objectRenderingProgram, "sf");
    glUniform1f(sfLoc, scaleFactor);

    for (int i = 0; i < numObjects; i++) {
        glVertexAttribPointer(0, 2, GL_FLOAT, false, 2 * sizeof(float), (void *)(2 * sizeof(float) * objectsOffset[i]));
        glEnableVertexAttribArray(0);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        glDrawArrays(GL_LINE_STRIP, 0, objects[i].vertices->size());
    }

    glfwSwapBuffers(window);
    glfwPollEvents();
}

void init(void) {
    for (int i = 0; i < numObjects; i++) {
        objects[i] = load2dObject(assets[i]);
    }

    particleRenderingProgram = Utils::createShaderProgram("shaders/particleVert.glsl", "shaders/particleFrag.glsl");
    objectRenderingProgram = Utils::createShaderProgram("shaders/objectVert.glsl", "shaders/objectFrag.glsl");
    computeProgram = Utils::createShaderProgram("shaders/collisionComputeShader.glsl");

    createParticles();

    curInBuffer = &buffer1[0];
    curOutBuffer = &buffer2[0];
    setupScene();
    setupComputeBuffers();
}

void runComputeShader(void) {
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
    glUniform1i(numEdgesLoc, numEdgesTotal);
    chunkWdithLoc = glGetUniformLocation(computeProgram, "chunkWidth");
    glUniform1f(chunkWdithLoc, chunks.chunkWidth);
    chunkHeightLoc = glGetUniformLocation(computeProgram, "chunkHeight");
    glUniform1f(chunkHeightLoc, chunks.chunkHeight);
    numChunksXLoc = glGetUniformLocation(computeProgram, "numChunksX");
    glUniform1i(numChunksXLoc, numChunksX);
    numChunksYLoc = glGetUniformLocation(computeProgram, "numChunksY");
    glUniform1i(numChunksYLoc, numChunksY);
    sfLoc = glGetUniformLocation(computeProgram, "sf");
    glUniform1f(sfLoc, scaleFactor);
    pptLoc = glGetUniformLocation(computeProgram, "particleProximityThreshold");
    glUniform1f(pptLoc, ppt);
    eeLoc = glGetUniformLocation(computeProgram, "numEdgeFloats");
    glUniform1i(eeLoc, numEdgeFloats);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, computeBuffers[0]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, computeBuffers[1]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, computeBuffers[2]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, computeBuffers[3]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, computeBuffers[4]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, computeBuffers[5]);
    glDispatchCompute(numParticlesX * numParticlesY / workGroupSize, 1, 1); 
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, computeBuffers[1]);
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(buffer1), curOutBuffer);

    updateChunkData(curOutBuffer);
}

void swapBuffers(void) {
    float *temp = curOutBuffer;
    curOutBuffer = curInBuffer;
    curInBuffer = temp;
}

double runFrame(GLFWwindow *window, double currentTime) {
    deltaTime = currentTime - pastTime;
    pastTime = currentTime;

    display(window);
    runComputeShader();
    swapBuffers();
    bindComputeBuffers();

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
    return deltaTime;
}

int writeBenchmarks(double avgFrameRate) {
    // Define the file name (relative to the source code directory)
    string fileName = "../../../benchmark.txt";

    ifstream inFile(fileName);
    double oldFrameRate = 0.0;

    if (inFile.is_open()) {
        string line;
        // Read the file line by line
        while (getline(inFile, line)) {
            if (line[0] == 'F') {
                sscanf(line.c_str(), "Frame Rate:   %lf", &oldFrameRate);
            }
        }

        // Close the file
        inFile.close();
    } else {
        cerr << "Unable to open file for reading." << endl;
        return 1;
    }

    // Create and open a text file
    std::ofstream outFile(fileName);

    // Check if the file was successfully opened
    if (outFile.is_open()) {
        // Write to the file
        outFile << "Benchmarks" << endl;
        outFile << "Particles:    " << numParticlesX * numParticlesY << endl;
        outFile << "Chunks:       " << numChunksX * numChunksY << endl;
        outFile << "Objects:      " << numObjects << endl;
        outFile << "Object Edges: " << numEdgesTotal << endl;
        outFile << "--------------" << endl;
        outFile << "Frame Rate:   " << avgFrameRate << endl;

        if (oldFrameRate != 0.0) {
            outFile << "Improvement:  " << (avgFrameRate - oldFrameRate) / oldFrameRate * 100.0 << "%" << endl;
        }

        // Close the file
        outFile.close();
        cout << "File written successfully." << endl;
    } else {
        cerr << "Unable to open file for writing." << endl;
    }

    return 0;
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
    double time = 0.0;
    int frames = 0;
    while (!glfwWindowShouldClose(window)) {
        time += runFrame(window, glfwGetTime());
        frames++;
    }
    glfwDestroyWindow(window);
    glfwTerminate();

    double avgFrameRate = (double)frames / time;
    cout << "Average frame rate: " << avgFrameRate << endl;
    
    if (benchmark) writeBenchmarks(avgFrameRate);

    exit(EXIT_SUCCESS);
}