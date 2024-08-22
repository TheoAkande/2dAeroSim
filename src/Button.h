#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <SOIL2/SOIL2.h>
#include <string>
#include <iostream>
#include <fstream>
#include <cmath>
#include <vector>
#include <functional>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define numButtonVAOs 1
#define numButtonVBOs 2

class Button
{
private:
    static bool initialized;
    static std::vector<Button *> buttons;
    static GLuint buttonShaderProgram;
    static GLuint emptyTexture;
    static int screenWidth, screenHeight;
    static float textureCoords[8];
    GLuint baseTexture, hoverTexture, clickTexture;
    GLuint bvao[numButtonVAOs];
    GLuint bvbo[numButtonVBOs];
    float buttonCoords[8];
    float buttonX, buttonY;
    int buttonXAbs, buttonYAbs, buttonWidth, buttonHeight;
    bool toggle, clicked, clickHeld;
    bool hasBaseTexture, hasHoverTexture, hasClickTexture;
    std::function<void (void*)> onClick;
    void *clickData;
    void updateButton(bool click, int mouseX, int mouseY);
public:
	Button(int x, int y, int width, int height, bool toggle, std::function<void (void*)> onClick, void *clickData);
    Button *withBaseTexture(const char *texture);
    Button *withHoverTexture(const char *texture);
    Button *withClickTexture(const char *texture);
    static void initButtons(int screenWidth, int screenHeight);
    static void update(bool click, int mouseX, int mouseY);
};