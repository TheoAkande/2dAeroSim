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

#ifndef SLIDER_H
#define SLIDER_H

#define numSliderVAOs 1
#define numSliderVBOs 2

enum class SliderType
{
    VERTICAL,
    HORIZONTAL
};

class Slider : public Updateable
{
    private:
        float value;
        float x, y, width, height;
        glm::vec4 barColour;
        glm::vec4 baseColour;
        GLuint svao[numSliderVAOs];
        GLuint svbo[numSliderVBOs];
        float baseVert[8];
        float barVert[8];
        SliderType type;
        void updateBar(void);
        void draw(void) override;
        void update(bool click, int mouseX, int mouseY) override;

        static GLuint sliderShaderProgram;
        static vector<Slider *> sliders;
        static bool initialized;
        static int screenWidth, screenHeight;
    public:
        Slider(
            int x, int y, int width, int height, float initialValue, 
            glm::vec4 barColour, glm::vec4 baseColour,
            SliderType type);
        float getValue(void);
        
        static void initSliders(void);
};

#endif