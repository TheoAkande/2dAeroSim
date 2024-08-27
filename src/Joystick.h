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
#include "Slider.h"

using namespace std;

#define numJoystickVAOs 1
#define numJoystickVBOs 2

class Joystick : public Updateable
{
    private:
        pair<float, float> value;
        float x, y, width, height;
        GLuint jvao[numJoystickVAOs];
        GLuint jvbo[numJoystickVBOs];
        glm::vec3 baseColour;
        glm::vec3 stickColour;
        float baseVert[8];
        float stickVert[8];
        Slider *xSlider, *ySlider;
        void updateStick(void);
        void draw(void) override;
        void update(bool click, int mouseX, int mouseY) override;

        static float textureCoords[8];
        static GLuint squareShaderProgram, circleShaderProgram;
        static bool initialized;
    public:
        Joystick(
            int x, int y, int width, int height, 
            glm::vec3 baseColour, glm::vec3 stickColour
        );
        pair<float, float> getValue(void);
        void setValue(pair<float, float> value);
        
        static void initJoysticks(void);
};