#include "TextRenderer.h"
#include "Utils.h"

using namespace std;

GLuint TextRenderer::numbersTexture;
GLuint TextRenderer::textShaderProgram;
GLuint TextRenderer::tvao[numTextVAOs];
GLuint TextRenderer::tvbo[numTextVBOs];
float TextRenderer::numOffsetX;
float TextRenderer::numOffsetY;
float TextRenderer::boxCoords[8];
float TextRenderer::texCoords[8];
bool TextRenderer::initialized = false;

void TextRenderer::initTextRenderer(int screenWidth, int screenHeight)
{
    if (initialized)
        return;

    numOffsetX = 20.0f / (float)screenWidth;
    numOffsetY = 30.0f / (float)screenHeight;

    TextRenderer::textShaderProgram = Utils::createShaderProgram("shaders/textureVert.glsl", "shaders/textureFrag.glsl");
    numbersTexture = Utils::loadTexture("assets/textures/numbers.jpg");

    glGenVertexArrays(numTextVAOs, tvao);
    glBindVertexArray(tvao[0]);
    glGenBuffers(numTextVBOs, tvbo);

    initialized = true;
}

TextRenderer::TextRenderer(int screenWidth, int screenHeight) {
    initTextRenderer(screenWidth, screenHeight);
}

void TextRenderer::renderInt(int num, float x, float y, float scale, TextAlignment alignment) {
    glUseProgram(TextRenderer::textShaderProgram);
    int digits = 0;
    int digVal = 1;
    while (num / digVal > 0) {
        digits++;
        digVal *= 10;
    }
    float digitBaseY = y - TextRenderer::numOffsetY * scale;
    for (int i = 0; i < digits; i++) {
        digVal /= 10;
        int intDigit = num / digVal;
        num = num % digVal;
        float digitBaseX = x + (float)i * TextRenderer::numOffsetX * scale;
        float digTexCoord = intDigit * 0.1f;
        boxCoords[0] = digitBaseX; 
        boxCoords[1] = digitBaseY;
        boxCoords[2] = digitBaseX + TextRenderer::numOffsetX * scale;
        boxCoords[3] = digitBaseY;
        boxCoords[4] = digitBaseX + TextRenderer::numOffsetX * scale;
        boxCoords[5] = digitBaseY + TextRenderer::numOffsetY * scale;
        boxCoords[6] = digitBaseX;
        boxCoords[7] = digitBaseY + TextRenderer::numOffsetY * scale;

        texCoords[0] = digTexCoord;
        texCoords[1] = 0.0f;
        texCoords[2] = digTexCoord + 0.1f;
        texCoords[3] = 0.0f;
        texCoords[4] = digTexCoord + 0.1f;
        texCoords[5] = 1.0f;
        texCoords[6] = digTexCoord;
        texCoords[7] = 1.0f;

        glBindBuffer(GL_ARRAY_BUFFER, tvbo[0]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(boxCoords), &boxCoords[0], GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, false, 2 * sizeof(float), (void *)0);
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, tvbo[1]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(texCoords), &texCoords[0], GL_STATIC_DRAW);
        glVertexAttribPointer(1, 2, GL_FLOAT, false, 2 * sizeof(float), (void *)0);
        glEnableVertexAttribArray(1);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, numbersTexture);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);        
    }

}