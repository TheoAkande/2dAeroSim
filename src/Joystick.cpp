#include "Joystick.h"
#include "Utils.h"

GLuint Joystick::squareShaderProgram, Joystick::circleShaderProgram;
bool Joystick::initialized = false;
float Joystick::textureCoords[8] = {
    0.0f, 0.0f,
    1.0f, 0.0f,
    1.0f, 1.0f,
    0.0f, 1.0f
};

void Joystick::updateStick(void) {
    this->stickVert[0] = this->x + this->value.first * this->width - this->width / 5.0f;
    this->stickVert[1] = this->y + this->value.second * this->height - this->height / 5.0f;
    this->stickVert[2] = this->x + this->value.first * this->width + this->width / 5.0f;
    this->stickVert[3] = this->y + this->value.second * this->height - this->height / 5.0f;
    this->stickVert[4] = this->x + this->value.first * this->width + this->width / 5.0f;
    this->stickVert[5] = this->y + this->value.second * this->height + this->height / 5.0f;
    this->stickVert[6] = this->x + this->value.first * this->width - this->width / 5.0f;
    this->stickVert[7] = this->y + this->value.second * this->height + this->height / 5.0f;

    glBindBuffer(GL_ARRAY_BUFFER, this->jvbo[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(this->stickVert), &this->stickVert[0], GL_STATIC_DRAW);

}

void Joystick::draw(void) {

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(Joystick::squareShaderProgram);

    glBindVertexArray(this->jvao[0]);
    this->updateStick();

    glBindBuffer(GL_ARRAY_BUFFER, this->jvbo[0]);
    glVertexAttribPointer(0, 2, GL_FLOAT, false, 2 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    GLuint colourLoc = glGetUniformLocation(Joystick::squareShaderProgram, "colourIn");
    glUniform3f(colourLoc, this->baseColour[0], this->baseColour[1], this->baseColour[2]);

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glUseProgram(Joystick::circleShaderProgram);

    colourLoc = glGetUniformLocation(Joystick::circleShaderProgram, "colourIn");
    glUniform3f(colourLoc, this->stickColour[0], this->stickColour[1], this->stickColour[2]);
    GLuint centreLoc = glGetUniformLocation(Joystick::circleShaderProgram, "centre");
    glUniform2f(centreLoc, this->x + this->value.first * this->width, this->y + this->value.second * this->height);
    GLuint radiusLoc = glGetUniformLocation(Joystick::circleShaderProgram, "radiusX");
    glUniform1f(radiusLoc, this->width / 5.0f);
    radiusLoc = glGetUniformLocation(Joystick::circleShaderProgram, "radiusY");
    glUniform1f(radiusLoc, this->height / 5.0f);

    glBindBuffer(GL_ARRAY_BUFFER, this->jvbo[1]);
    glVertexAttribPointer(0, 2, GL_FLOAT, false, 2 * sizeof(float), (void *)0);

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void Joystick::update(bool click, int mouseX, int mouseY) {
    this->value.first = this->xSlider->getValue();
    this->value.second = this->ySlider->getValue();
    this->updateStick();
}

Joystick::Joystick(
    int x, int y, int width, int height,
    glm::vec3 baseColour, glm::vec3 stickColour
) : Updateable() {
    Joystick::initJoysticks();

    this->xSlider = new Slider(
        x, y, width, height, 0.5f, 
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
        SliderType::HORIZONTAL
    );
    this->xSlider->setInvisible();
    this->ySlider = new Slider(
        x, y, width, height, 0.5f, 
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
        SliderType::VERTICAL
    );
    this->ySlider->setInvisible();

    this->x = Utils::pixelToScreenX(x);
    this->y = Utils::pixelToScreenY(y);
    this->width = Utils::pixelsToScreenWidth(width);
    this->height = Utils::pixelsToScreenHeight(height);
    this->baseColour = baseColour;
    this->stickColour = stickColour;

    this->baseVert[0] = this->x;
    this->baseVert[1] = this->y;
    this->baseVert[2] = this->x + this->width;
    this->baseVert[3] = this->y;
    this->baseVert[4] = this->x + this->width;
    this->baseVert[5] = this->y + this->height;
    this->baseVert[6] = this->x;
    this->baseVert[7] = this->y + this->height;

    glGenVertexArrays(numJoystickVAOs, this->jvao);
    glBindVertexArray(this->jvao[0]);
    glGenBuffers(numJoystickVBOs, this->jvbo);

    glBindBuffer(GL_ARRAY_BUFFER, this->jvbo[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(this->baseVert), &this->baseVert[0], GL_STATIC_DRAW);

    this->updateStick();
}

pair<float, float> Joystick::getValue(void) {
    return this->value;
}

void Joystick::initJoysticks(void) {
    if (Joystick::initialized) return;

    Joystick::squareShaderProgram = Utils::createShaderProgram("shaders/sliderVert.glsl", "shaders/sliderFrag.glsl");
    Joystick::circleShaderProgram = Utils::createShaderProgram("shaders/circleVert.glsl", "shaders/circleFrag.glsl");

    Joystick::initialized = true;
}