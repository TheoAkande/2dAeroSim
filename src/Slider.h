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

#define numSliderVAOs 1
#define numSliderVBOs 1

class Slider
{
    private:
        float value;
        float x, y, width, height;
        glm::vec4 barColour;
        glm::vec4 baseColour;
        GLuint svao[numSliderVAOs];
        GLuint svbo[numSliderVBOs];
        bool active;
        void drawSlider(void);
        void updateSlider(bool click, int mouseX, int mouseY);

        static GLuint sliderShaderProgram;
        static vector<Slider *> sliders;
        static bool initialized;
        static int screenWidth, screenHeight;
    public:
        Slider(float x, float y, float width, float height, float initialValue, glm::vec4 barColour, glm::vec4 baseColour);
        float getValue(void);
        
        static void initSliders(int screenWidth, int screenHeight);
        static void update(bool click, int mouseX, int mouseY);
};