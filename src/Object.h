#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <SOIL2/SOIL2.h>
#include <string>
#include <iostream>
#include <fstream>
#include <cmath>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "EntityManager.h"

using namespace std;

#define numObjectVAOs 1
#define numObjectVBOs 1
#define edgeElasticity 0.3f
#define numEdgeFloats 7

struct Line {
    float x1, y1;
    float x2, y2;
    float nx, ny;
    float elasticity;
};

class Object : public Entity
{
    private:
        vector<glm::vec2> *vertices;
        vector<Line> *edges;
        float mass, scale;
        float x, y;
        float elasticity;
        float colour[3];
        GLuint ovao[numObjectVAOs];
        GLuint ovbo[numObjectVBOs];
        glm::mat4 viewMat;
        void setupDraw(void);
        void draw(void) override;

        static GLuint objectShaderProgram;
        static vector<Object *> objects;
        static bool initialized;
        static int screenWidth, screenHeight, simulationWidth, simulationHeight;
        static float scaleX, scaleY;
        static float scaleFactor;
    public:
        int numEdges;
        int numVertices;
        Object(const char *path);
        Object(vector<glm::vec2> *vertices);
        void setMass(float mass);
        void setScale(float scale);
        void setColour(float r, float g, float b);
        void translate(float x, float y);
        void setActive(void);
        void setInactive(void);
        void printObject(void);
        
        static int numObjects;
        static int loadAllEdges(vector<float> *edges);
        static void initObjects(int screenWidth, int screenHeight, float scaleFactor, float simulationWidth, float simulationHeight);
};