#include "Button.h"
#include "Utils.h"

using namespace std;

bool Button::initialized = false;
std::vector<Button *> Button::buttons;
int Button::screenWidth, Button::screenHeight;
float Button::textureCoords[8];
GLuint Button::emptyTexture;
GLuint Button::buttonShaderProgram;

void Button::drawButton(GLuint texture) {
    glUseProgram(Button::buttonShaderProgram);

    glBindBuffer(GL_ARRAY_BUFFER, this->bvbo[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(this->buttonCoords), &(this->buttonCoords[0]), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, false, 2 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, this->bvbo[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Button::textureCoords), &(this->textureCoords[0]), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 2, GL_FLOAT, false, 2 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(1);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void Button::updateButton(bool clickOn, int mouseX, int mouseY) {
    bool hover = 
        this->buttonXAbs <= mouseX && buttonXAbs + buttonWidth >= mouseX &&
        this->buttonYAbs <= mouseY && buttonYAbs + buttonHeight >= mouseY;

    bool click = hover && clickOn;

    if (click && !this->clickHeld) {
        this->clicked = true;
        this->onClick(clickData);
    }
    
    GLuint activeTexture = this->hasBaseTexture ? this->baseTexture : Button::emptyTexture;

    if (hover && this->hasHoverTexture) {
        activeTexture = this->hoverTexture;
    } else if (this->clicked && this->hasClickTexture) {
        activeTexture = this->clickTexture;
    }

    this->clickHeld = clickOn;

    this->drawButton(activeTexture);
}

void Button::initButtons(int screenWidth, int screenHeight) {
    if (Button::initialized) return;

    Button::buttonShaderProgram = Utils::createShaderProgram("shaders/textureVert.glsl", "shaders/textureFrag.glsl");
    Button::emptyTexture = Utils::loadTexture("assets/textures/emptyButton.jpg");
    Button::buttons = std::vector<Button *>();
    Button::screenHeight = screenHeight;
    Button::screenWidth = screenWidth;
    Button::textureCoords[0] = 0.0f;
    Button::textureCoords[1] = 0.0f;
    Button::textureCoords[2] = 1.0f;
    Button::textureCoords[3] = 0.0f;
    Button::textureCoords[4] = 1.0f;
    Button::textureCoords[5] = 1.0f;
    Button::textureCoords[6] = 0.0f;
    Button::textureCoords[7] = 1.0f;
    Button::initialized = true;
}

Button::Button(int x, int y, int width, int height, std::function<void (void*)> onClick, void *clickData) {
    this->buttonXAbs = x;
    this->buttonYAbs = y;
    this->buttonX = (x / (float)Button::screenWidth) * 2.0f - 1.0f;
    this->buttonY = (y / (float)Button::screenHeight) * 2.0f - 1.0f;
    this->buttonWidth = width;
    this->buttonHeight = height;
    this->onClick = onClick;
    this->clickData = clickData;
    this->hasBaseTexture = false;
    this->hasClickTexture = false;
    this->hasHoverTexture = false;
    this->clicked = false;

    float decimalWidth = (width / (float)Button::screenWidth) * 2.0f;
    float decimalHeight = (height / (float)Button::screenHeight) * 2.0f;

    this->buttonCoords[0] = this->buttonX;
    this->buttonCoords[1] = this->buttonY;
    this->buttonCoords[2] = this->buttonX + decimalWidth;
    this->buttonCoords[3] = this->buttonY;
    this->buttonCoords[4] = this->buttonX + decimalWidth;
    this->buttonCoords[5] = this->buttonY + decimalHeight;
    this->buttonCoords[6] = this->buttonX;
    this->buttonCoords[7] = this->buttonY + decimalHeight;

    glGenVertexArrays(numButtonVAOs, this->bvao);
    glBindVertexArray(this->bvao[0]);
    glGenBuffers(numButtonVBOs, this->bvbo);

    glBindBuffer(GL_ARRAY_BUFFER, this->bvbo[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(this->buttonCoords), &(this->buttonCoords[0]), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, false, 2 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, this->bvbo[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Button::textureCoords), &(this->textureCoords[0]), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 2, GL_FLOAT, false, 2 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(1);

    Button::buttons.push_back(this);
}

Button *Button::withBaseTexture(const char *texture) {
    this->baseTexture = Utils::loadTexture(texture);
    this->hasBaseTexture = true;
    return this;
}

Button *Button::withHoverTexture(const char *texture) {
    this->hoverTexture = Utils::loadTexture(texture);
    this->hasHoverTexture = true;
    return this;
}

Button *Button::withClickTexture(const char *texture) {
    this->clickTexture = Utils::loadTexture(texture);
    this->hasClickTexture = true;
    return this;
}

void Button::update(bool click, int mouseX, int mouseY) {
    for (int i = 0; i < Button::buttons.size(); i++) {
        Button::buttons.at(i)->updateButton(click, mouseX, mouseY);
    }
}

ToggleButton::ToggleButton(
    int x, int y, int width, int height, 
    std::function<void (void*)> onToggleOn, void *toggleOnData, 
    std::function<void (void*)> onToggleOff, void *toggleOffData
) : Button(x, y, width, height, onToggleOn, toggleOnData) {
    this->toggleOff = onToggleOff;
    this->toggleOffData = toggleOffData;
    this->toggled = false;
}

void ToggleButton::updateButton(bool clickOn, int mouseX, int mouseY) {
    bool hover = 
        this->buttonXAbs <= mouseX && buttonXAbs + buttonWidth >= mouseX &&
        this->buttonYAbs <= mouseY && buttonYAbs + buttonHeight >= mouseY;

    bool click = hover && clickOn;

    if (click && !this->clickHeld) {
        this->toggled = !this->toggled;
        if (this->toggled) {
            this->onClick(clickData);
        } else {
            this->toggleOff(toggleOffData);
        }
    }
    
    GLuint activeTexture = this->hasBaseTexture ? this->baseTexture : Button::emptyTexture;

    if (this->toggled && this->hasClickTexture) {
        activeTexture = this->clickTexture;
    }

    this->clickHeld = clickOn;

    this->drawButton(activeTexture);
}