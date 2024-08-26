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

#define numTextVAOs 1
#define numTextVBOs 2

enum class TextAlignment
{
    LEFT,
    RIGHT
};

class TextRenderer
{
private:
    static bool initialized;
    static GLuint numbersTexture;
    static GLuint textShaderProgram;
    static GLuint tvao[numTextVAOs];
    static GLuint tvbo[numTextVBOs];
    static float numOffsetX, numOffsetY;
    static float boxCoords[8];
    static float texCoords[8];
public:
	TextRenderer(int, int);
    static void initTextRenderer(int screenWidth, int screenHeight);
	static void renderInt(int num, int x, int y, float scale, TextAlignment alignment);
};