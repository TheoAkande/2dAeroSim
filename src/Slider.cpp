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

void Slider::updateSlider(bool click, int mouseX, int mouseY) {
    if (click) {
        float adjustedMouseX = ((float)mouseX / (float)Slider::screenWidth) * 2.0f - 1.0f;
        float adjustedMouseY = (((float)mouseY / (float)Slider::screenHeight) * 2.0f - 1.0f);
        if (
            adjustedMouseX >= this->x 
            && adjustedMouseX <= this->x + this->width 
            && adjustedMouseY >= this->y 
            && adjustedMouseY <= this->y + this->height
        ) {
            this->value = this->type == SliderType::HORIZONTAL 
                ? (adjustedMouseX - this->x) / this->width 
                : (adjustedMouseY - this->y) / this->height;
            if (this->value < 0.0f) this->value = 0.0f;
            if (this->value > 1.0f) this->value = 1.0f;
            this->updateBar();
        } 
    }

    this->drawSlider();
}

Slider::Slider(
    int x, int y, int width, int height, float initialValue, 
    glm::vec4 barColour, glm::vec4 baseColour,
    SliderType type) {
    this->x = ((float)x / (float)Slider::screenWidth) * 2.0f - 1.0f;
    this->y = (((float)y / (float)Slider::screenHeight)) * 2.0f - 1.0f;
    this->width = ((float)width / (float)Slider::screenWidth) * 2.0f;
    this->height = ((float)height / (float)Slider::screenHeight) * 2.0f;
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