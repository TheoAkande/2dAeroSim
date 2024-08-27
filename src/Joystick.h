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
        float value;
        float x, y, width, height;
        GLuint baseTexture, stickTexture;
        GLuint jvao[numJoystickVAOs];
        GLuint jvbo[numJoystickVBOs];
        float baseVert[8];
        float stickVert[8];
        Slider *xSlider, *ySlider;
        void updateStick(void);
        void draw(void) override;
        void update(bool click, int mouseX, int mouseY) override;

        static GLuint jsShaderProgram;
        static bool initialized;
        static int screenWidth, screenHeight;
    public:
        Joystick(
            int x, int y, int width, int height, 
            const char *baseTexture, const char *stickTexture
        );
        pair<float, float> getValue(void);
        
        static void initJoysitcks(void);
};