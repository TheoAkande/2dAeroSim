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
        glm::vec4 colour;
        GLuint ovao[numObjectVAOs];
        GLuint ovbo[numObjectVBOs];
        bool active;
        void drawObject(void);

        static GLuint objectShaderProgram;
        static vector<Object *> objects;
        static bool initialized;
        static int screenWidth, screenHeight;
    public:
        int numEdges;
        int numVertices;
        Object(const char *path);
        Object(vector<glm::vec2> *vertices);
        void setMass(float mass);
        void setScale(float scale);
        void translate(float x, float y);
        
        static void initObjects(int screenWidth, int screenHeight);
        static void update(void);
};