#include "Button.h"
#include "Utils.h"

using namespace std;

bool Button::initialized = false;
std::vector<Button *> Button::buttons;
float Button::textureCoords[8];
GLuint Button::emptyTexture;
GLuint Button::buttonShaderProgram;

void Button::draw(void) {
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
    glBindTexture(GL_TEXTURE_2D, this->currentTexture);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void Button::update(bool clickOn, int mouseX, int mouseY) {
    bool hover = 
        this->buttonXAbs <= mouseX && buttonXAbs + buttonWidth >= mouseX &&
        this->buttonYAbs <= mouseY && buttonYAbs + buttonHeight >= mouseY;

    bool click = hover && clickOn;

    if (click && !this->clickHeld) {
        this->onClick(clickData);
    }
    
    this->currentTexture = this->hasBaseTexture ? this->baseTexture : Button::emptyTexture;

    if (click && this->hasClickTexture) {
        this->currentTexture = this->clickTexture;
    } else if (hover && this->hasHoverTexture) {
        this->currentTexture = this->hoverTexture;
    }

    this->clickHeld = clickOn;
}

void Button::initButtons(void) {
    if (Button::initialized) return;

    Button::buttonShaderProgram = Utils::createShaderProgram("shaders/textureVert.glsl", "shaders/textureFrag.glsl");
    Button::emptyTexture = Utils::loadTexture("assets/textures/emptyButton.jpg");
    Button::buttons = std::vector<Button *>();
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
    Button::initButtons();

    this->buttonXAbs = x;
    this->buttonYAbs = y;
    this->buttonX = Utils::pixelToScreenX(x);
    this->buttonY = Utils::pixelToScreenY(y);
    this->buttonWidth = width;
    this->buttonHeight = height;
    this->onClick = onClick;
    this->clickData = clickData;
    this->hasBaseTexture = false;
    this->hasClickTexture = false;
    this->hasHoverTexture = false;

    float decimalWidth = Utils::pixelsToScreenWidth(width);
    float decimalHeight = Utils::pixelsToScreenHeight(height);

    this->buttonCoords[0] = this->buttonX;
    this->buttonCoords[1] = this->buttonY;
    this->buttonCoords[2] = this->buttonX + decimalWidth;
    this->buttonCoords[3] = this->buttonY;
    this->buttonCoords[4] = this->buttonX + decimalWidth;
    this->buttonCoords[5] = this->buttonY + decimalHeight;
    this->buttonCoords[6] = this->buttonX;
    this->buttonCoords[7] = this->buttonY + decimalHeight;

    this->baseTexture = Button::emptyTexture;

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

ToggleButton::ToggleButton(
    int x, int y, int width, int height, 
    std::function<void (void*)> onToggleOn, void *toggleOnData, 
    std::function<void (void*)> onToggleOff, void *toggleOffData
) : Button(x, y, width, height, onToggleOn, toggleOnData) {
    this->toggleOff = onToggleOff;
    this->toggleOffData = toggleOffData;
    this->toggled = false;
}

void ToggleButton::update(bool clickOn, int mouseX, int mouseY) {
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
    
    this->currentTexture = this->hasBaseTexture ? this->baseTexture : Button::emptyTexture;

    if (this->toggled && this->hasClickTexture) {
        this->currentTexture = this->clickTexture;
    }

    this->clickHeld = clickOn;
}