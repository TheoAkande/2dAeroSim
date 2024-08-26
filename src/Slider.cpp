#include "Slider.h"
#include "Utils.h"

GLuint Slider::sliderShaderProgram;
vector<Slider *> Slider::sliders;
bool Slider::initialized = false;
int Slider::screenWidth, Slider::screenHeight;

void Slider::updateBar(void) {
    if (this->type == SliderType::HORIZONTAL) {
        this->barVert[0] = this->x;
        this->barVert[1] = this->y;
        this->barVert[2] = this->x + this->width * this->value;
        this->barVert[3] = this->y;
        this->barVert[4] = this->x + this->width * this->value;
        this->barVert[5] = this->y + this->height;
        this->barVert[6] = this->x;
        this->barVert[7] = this->y + this->height;
    } else {
        this->barVert[0] = this->x;
        this->barVert[1] = this->y;
        this->barVert[2] = this->x;
        this->barVert[3] = this->y + this->height * this->value;
        this->barVert[4] = this->x + this->width;
        this->barVert[5] = this->y + this->height * this->value;
        this->barVert[6] = this->x + this->width;
        this->barVert[7] = this->y;
    }

    glBindBuffer(GL_ARRAY_BUFFER, this->svbo[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(this->barVert), &this->barVert[0], GL_STATIC_DRAW);
}

void Slider::drawSlider(void) {
    glUseProgram(Slider::sliderShaderProgram);

    glBindVertexArray(this->svao[0]);
    glEnableVertexAttribArray(0);

    GLuint colourLoc = glGetUniformLocation(Slider::sliderShaderProgram, "colourIn");
    glUniform3f(colourLoc, this->baseColour[0], this->baseColour[1], this->baseColour[2]);

    glBindBuffer(GL_ARRAY_BUFFER, this->svbo[0]);
    glVertexAttribPointer(0, 2, GL_FLOAT, false, 2 * sizeof(float), (void *)0);

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glUniform3f(colourLoc, this->barColour[0], this->barColour[1], this->barColour[2]);

    glBindBuffer(GL_ARRAY_BUFFER, this->svbo[1]);
    glVertexAttribPointer(0, 2, GL_FLOAT, false, 2 * sizeof(float), (void *)0);

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

Slider::Slider(
    float x, float y, float width, float height, float initialValue, 
    glm::vec4 barColour, glm::vec4 baseColour,
    SliderType type) {
    this->x = x;
    this->y = y;
    this->width = width;
    this->height = height;
    this->value = initialValue;
    this->barColour = barColour;
    this->baseColour = baseColour;
    this->type = type;
    this->active = true;

    this->baseVert[0] = x;
    this->baseVert[1] = y;
    this->baseVert[2] = x + width;
    this->baseVert[3] = y;
    this->baseVert[4] = x + width;
    this->baseVert[5] = y + height;
    this->baseVert[6] = x;
    this->baseVert[7] = y + height;

    glGenVertexArrays(numSliderVAOs, this->svao);
    glBindVertexArray(this->svao[0]);
    glGenBuffers(numSliderVBOs, this->svbo);

    glBindBuffer(GL_ARRAY_BUFFER, this->svbo[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(this->baseVert), &this->baseVert[0], GL_STATIC_DRAW);

    this->updateBar();

    glBindVertexArray(0);

    Slider::sliders.push_back(this);
}

float Slider::getValue(void) {
    return this->value;
}

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