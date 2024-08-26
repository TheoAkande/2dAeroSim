#include "Slider.h"
#include "Utils.h"

GLuint Slider::sliderShaderProgram;
vector<Slider *> Slider::sliders;
bool Slider::initialized = false;
int Slider::screenWidth, Slider::screenHeight;


void Slider::initSliders(int screenWidth, int screenHeight) {
    if (Slider::initialized) return;

    Slider::screenWidth = screenWidth;
    Slider::screenHeight = screenHeight;

    Slider::sliderShaderProgram = Utils::createShaderProgram("shaders/sliderVert.glsl", "shaders/sliderFrag.glsl");

    Slider::sliders = vector<Slider *>();

    Slider::initialized = true;
}

void Slider::update(bool click, int mouseX, int mouseY) {
    for (int i = 0; i < Slider::sliders.size(); i++) {
        if (Slider::sliders.at(i)->active) Slider::sliders.at(i)->updateSlider(click, mouseX, mouseY);
    }
}