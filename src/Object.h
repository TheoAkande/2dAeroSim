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

using namespace std;

#define numObjectVAOs 1
#define numObjectVBOs 1
#define edgeElasticity 0.3f

struct Line {
    float x1, y1;
    float x2, y2;
    float nx, ny;
    float elasticity;
};

class Object
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
        bool active;
        glm::mat4 viewMat;
        void setupDraw(void);
        void drawObject(void);

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
        
        static void initObjects(int screenWidth, int screenHeight, float scaleFactor, float simulationWidth, float simulationHeight);
        static void update(void);
};