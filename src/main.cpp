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

#include "TextRenderer.h"
#include "Button.h"
#include "Object.h"
#include "Slider.h"
#include "Joystick.h"
#include "Utils.h"
#include "EntityManager.h"

using namespace std;

#define benchmark false

#define numParticlesX 128
#define numParticlesY 128
#define particleMass 1.0f
#define rangeOfMotion 300.0f
#define vMax 5000.0f
#define colourVMax 300.0f
#define numParticleFloats 8
#define force 10000.0f
#define particleElasticity 0.975f

#define scaleFactor 1080.0f
#define numChunksX 32
#define numChunksY 32
#define ppt 1.5f

#define numVBOs 1
#define numVAOs 1
#define numCBs 6
#define workGroupSize 64

#define windowWidth 2000
#define windowHeight 1500
#define simulationWidth 1000
#define simulationHeight 1000

#define FPSx 100
#define FPSy 1400

pair<float, float> constantForce = {0.0f, -20000.0f};

bool doSimulation = false;
bool spaceHeld = false;

double pastTime = 0.0;
double deltaTime = 0.0;
double lastFPSUpdate;
int frames;
int framesTotal;
int lastFrames;
glm::mat4 viewMat;
float scaleX, scaleY;

int numParticles = numParticlesX * numParticlesY;
int numEdgesTotal;

// for window
int height = 1000;
int width = 1000;
GLuint vao[numVAOs];
GLuint vbo[numVBOs];

GLuint numbersTexture;

// for compute shader
GLuint  floatNumLoc, romLoc, vMaxLoc, dtLoc, xForceLoc, yForceLoc, numEdgesLoc, 
        sfLoc, tvmsLoc, chunkWdithLoc, chunkHeightLoc, numChunksXLoc, numChunksYLoc,
        pptLoc, pfLoc, eeLoc, peLoc, vMatLoc;
GLuint computeBuffers[numCBs];
GLuint particleRenderingProgram, computeProgram, textureRenderingProgram;
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

struct Chunks {
    int chunks[numParticlesX * numParticlesY];
    int cumChunkSizes[numChunksX * numChunksY];
    int chunkSizes[numChunksX * numChunksY];
    int chunksCounter[numChunksX * numChunksY];
    float chunkWidth, chunkHeight;
};

Particle particles[numParticlesX * numParticlesY];
Chunks chunks = Chunks();
double curFPS;
Button *myFirstButton, *mySecondButton;
Object *square, *triangle, *inverted;
Slider *myFirstSlider, *mySecondSlider;
Joystick *myJoystick;

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

    glGenVertexArrays(numVAOs, vao);
    glBindVertexArray(vao[0]);
    glGenBuffers(numVBOs, vbo);
    
    // Particles VBO
    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(buffer1), &curInBuffer[0], GL_STATIC_DRAW);
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
    numEdgesTotal = Object::loadAllEdges(&objectEdges);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, computeBuffers[2]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, numEdgesTotal * numEdgeFloats * sizeof(float), objectEdges.data(), GL_STATIC_DRAW);
}

void baseDisplay(GLFWwindow *window) {
    glClear(GL_DEPTH_BUFFER_BIT);
    glClear(GL_COLOR_BUFFER_BIT);

    // Draw particles
    glUseProgram(particleRenderingProgram);
    glPointSize(3.0f);
    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(buffer1), &curInBuffer[0], GL_STATIC_DRAW);

    sfLoc = glGetUniformLocation(particleRenderingProgram, "sf");
    glUniform1f(sfLoc, scaleFactor);
    tvmsLoc = glGetUniformLocation(particleRenderingProgram, "totalVMax");
    glUniform1f(tvmsLoc, colourVMax);
    vMatLoc = glGetUniformLocation(particleRenderingProgram, "viewMat");
    glUniformMatrix4fv(vMatLoc, 1, GL_FALSE, glm::value_ptr(viewMat));

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, numParticleFloats * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, numParticleFloats * sizeof(float), (void *)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDrawArrays(GL_POINTS, 0, numParticlesX * numParticlesY);

    // Show fps
    TextRenderer::renderInt((int)curFPS, FPSx, FPSy, 2.0f, TextAlignment::RIGHT);

    // Draw buttons
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    bool pressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    Updateable::updateEntities(pressed, (int)xpos, windowHeight - (int)ypos);
    Entity::drawEntities();

    TextRenderer::renderInt((int)(myFirstSlider->getValue() * 100), 10, 165, 3.0f, TextAlignment::LEFT);
    TextRenderer::renderInt((int)(mySecondSlider->getValue() * 100), 225, 10, 3.0f, TextAlignment::LEFT);

    glfwSwapBuffers(window);
    glfwPollEvents();
}

void buttonClick(void *param) {
    cout << "click!" << endl;
    return;
}

void switchSimulationState(void *param) {
    doSimulation = !doSimulation;
}

void init(void) {

    Utils::setScreenDimensions(windowWidth, windowHeight);
    TextRenderer::initTextRenderer(windowWidth, windowHeight);
    Object::initObjects(windowWidth, windowHeight, scaleFactor, simulationWidth, simulationHeight);

    myFirstButton = new Button(500, 1300, 300, 150, buttonClick, nullptr);
    myFirstButton->withHoverTexture("assets/textures/test.jpg");
    myFirstButton->withClickTexture("assets/textures/numbers.jpg");
    myFirstButton->setInvisible();
    myFirstButton->setInactive();
    mySecondButton = new ToggleButton(1000, 1300, 300, 150, switchSimulationState, nullptr, switchSimulationState, nullptr);
    mySecondButton->withBaseTexture("assets/textures/resumeSimulation.jpg");
    mySecondButton->withClickTexture("assets/textures/pauseSimulation.jpg");

    square = new Object("assets/objects/box.2dObj");
    triangle = new Object("assets/objects/triangle.2dObj");
    inverted = new Object("assets/objects/inverted.2dObj");

    myFirstSlider = new Slider(10, 10, 50, 150, 0.5f, glm::vec4(0.0f, 1.0f, 0.0f, 1.0f), glm::vec4(0.0f, 0.0f, 1.0f, 1.0f), SliderType::VERTICAL);
    mySecondSlider = new Slider(70, 10, 150, 50, 0.5f, glm::vec4(0.0f, 1.0f, 0.0f, 1.0f), glm::vec4(0.0f, 0.0f, 1.0f, 1.0f), SliderType::HORIZONTAL);

    triangle->setColour(0.0f, 1.0f, 0.0f);

    myJoystick = new Joystick(1500, 100, 150, 150, "assets/textures/js.png", "assets/textures/stick.png");

    lastFPSUpdate = 0.0l;
    curFPS = 1.0l;

    numbersTexture = Utils::loadTexture("assets/textures/numbers.jpg");

    scaleX = (float)simulationWidth / (float)windowWidth;
    scaleY = (float)simulationHeight / (float)windowHeight;
    viewMat = glm::scale(glm::mat4(1.0f), glm::vec3(scaleX, scaleY, 0.0f));

    particleRenderingProgram = Utils::createShaderProgram("shaders/particleVert.glsl", "shaders/particleFrag.glsl");
    textureRenderingProgram = Utils::createShaderProgram("shaders/textureVert.glsl", "shaders/textureFrag.glsl");
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
    peLoc = glGetUniformLocation(computeProgram, "particleElasticity");
    glUniform1f(peLoc, particleElasticity);

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

    if (currentTime - lastFPSUpdate > 1.0) {
        curFPS = (framesTotal - lastFrames) / (currentTime - lastFPSUpdate);
        lastFPSUpdate = currentTime;
        lastFrames = framesTotal;
    }

    baseDisplay(window);

    if (doSimulation) {
        runComputeShader();
        swapBuffers();
        bindComputeBuffers();

        xForce = constantForce.first;
        yForce = constantForce.second;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        xForce = constantForce.first - force;
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        xForce = constantForce.first + force;
    }
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        yForce = constantForce.second + force;
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        yForce = constantForce.second - force;
    }
    if (!doSimulation) {
        return -1;
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
    ofstream outFile(fileName);

    // Check if the file was successfully opened
    if (outFile.is_open()) {
        // Write to the file
        outFile << "Benchmarks" << endl;
        outFile << "Particles:    " << numParticlesX * numParticlesY << endl;
        outFile << "Chunks:       " << numChunksX * numChunksY << endl;
        outFile << "Objects:      " << Object::numObjects << endl;
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
    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "2dAeroSim", NULL, NULL);
    glfwMakeContextCurrent(window);
    if (glewInit() != GLEW_OK) { 
        exit(EXIT_FAILURE); 
    }
    glfwSwapInterval(1);
    init();
    double time = 0.0;
    frames = 0;
    double lastTime = 0.0;
    while (!glfwWindowShouldClose(window)) {
        lastTime = runFrame(window, glfwGetTime());
        if (lastTime > 0) {
            time += lastTime;
            frames++;
        }
        framesTotal++;
    }
    glfwDestroyWindow(window);
    glfwTerminate();

    if (time > 0) {
        double avgFrameRate = (double)frames / time;
        cout << "Average frame rate: " << avgFrameRate << endl;
        
        if (benchmark) writeBenchmarks(avgFrameRate);
    } else {
        cout << "No frames run." << endl;
    }

    exit(EXIT_SUCCESS);
}