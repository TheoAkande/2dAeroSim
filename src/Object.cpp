#include "Object.h"
#include "Utils.h"

GLuint Object::objectShaderProgram;
vector<Object *> Object::objects;
bool Object::initialized = false;
int Object::screenWidth, Object::screenHeight;
float Object::scaleFactor;
int Object::simulationWidth, Object::simulationHeight;
float Object::scaleX, Object::scaleY;
int Object::numObjects = 0;

Object::Object(const char *path) {
    if (!Object::initialized) {
        cerr << "Unable to create object when Objects not initialized." << endl;
        return;
    }

    this->vertices = new vector<glm::vec2>();
    this->edges = new vector<Line>();

    this->x = 0.0f;
    this->y = 0.0f;
    this->mass = 1.0f;
    this->scale = 1.0f;
    this->elasticity = edgeElasticity;

    int nv = 0;

    ifstream fileStream(path, ios::in);
    string line = "";
    while (!fileStream.eof()) {
        getline(fileStream, line);
        if (line.c_str()[0] == 'v') {
            glm::vec2 vertex;
            sscanf(line.c_str(), "v %f %f", &vertex.x, &vertex.y);
            vertices->push_back(glm::vec2(vertex.x * scaleFactor, vertex.y * scaleFactor));
            nv++;
        } else if (line.c_str()[0] == 'm') {
            sscanf(line.c_str(), "m %f", &this->mass);
        } else if (line.c_str()[0] == 's') {
            sscanf(line.c_str(), "s %f", &this->scale);
        } else if (line.c_str()[0] == 'e') {
            sscanf(line.c_str(), "e %f", &this->elasticity);
        }
    }
    fileStream.close();

    for (int i = 0; i < vertices->size() - 1; i++) {
        Line edge;
        edge.x1 = vertices->at(i).x;
        edge.y1 = vertices->at(i).y;
        edge.x2 = vertices->at((i + 1)).x;
        edge.y2 = vertices->at((i + 1)).y;
        edge.nx = edge.y1 - edge.y2;
        edge.ny = edge.x2 - edge.x1;
        edge.elasticity = this->elasticity;
        edges->push_back(edge);
    }

    this->active = true;
    this->numVertices = nv;
    this->numEdges = nv - 1;
    this->edges = edges;
    this->colour[0] = 1.0f;
    this->colour[1] = 0.0f;
    this->colour[2] = 0.0f;
    this->viewMat = glm::scale(glm::mat4(1.0f), glm::vec3(scaleX, scaleY, 0.0f));
    // this->viewMat *= glm::translate(glm::mat4(1.0f), glm::vec3(this->x * Object::scaleX, this->y * Object::scaleY, 0.0f));

    Object::numObjects++;

    Object::objects.push_back(this);

    this->setupDraw();
}

Object::Object(vector<glm::vec2> *vertices) {
    this->vertices = vertices;
    this->edges = new vector<Line>();

    this->x = 0.0f;
    this->y = 0.0f;
    this->mass = 1.0f;
    this->scale = 1.0f;
    this->elasticity = edgeElasticity;

    int nv = vertices->size();

    for (int i = 0; i < nv - 1; i++) {
        Line edge;
        edge.x1 = vertices->at(i).x;
        edge.y1 = vertices->at(i).y;
        edge.x2 = vertices->at((i + 1)).x;
        edge.y2 = vertices->at((i + 1)).y;
        edge.nx = edge.y1 - edge.y2;
        edge.ny = edge.x2 - edge.x1;
        edge.elasticity = this->elasticity;
        edges->push_back(edge);
    }

    this->active = true;
    this->numVertices = nv;
    this->numEdges = nv - 1;
    this->edges = edges;
    this->colour[0] = 1.0f;
    this->colour[1] = 0.0f;
    this->colour[2] = 0.0f;
    this->viewMat = glm::scale(glm::mat4(1.0f), glm::vec3(scaleX, scaleY, 0.0f));
    // this->viewMat *= glm::translate(glm::mat4(1.0f), glm::vec3(this->x * Object::scaleX, this->y * Object::scaleY, 0.0f));

    Object::numObjects++;

    Object::objects.push_back(this);

    this->setupDraw();
}

void Object::setupDraw(void) {
    glGenVertexArrays(numObjectVAOs, this->ovao);
    glBindVertexArray(this->ovao[0]);
    glGenBuffers(numObjectVBOs, this->ovbo);

    glBindBuffer(GL_ARRAY_BUFFER, this->ovbo[0]);
    glBufferData(GL_ARRAY_BUFFER, this->vertices->size() * sizeof(float), this->vertices->data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, false, 2 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
}

void Object::drawObject(void) {
    glUseProgram(Object::objectShaderProgram);
    glBindBuffer(GL_ARRAY_BUFFER, this->ovbo[0]);

    GLuint sfLoc = glGetUniformLocation(Object::objectShaderProgram, "sf");
    glUniform1f(sfLoc, Object::scaleFactor);
    GLuint vMatLoc = glGetUniformLocation(Object::objectShaderProgram, "viewMat");
    glUniformMatrix4fv(vMatLoc, 1, GL_FALSE, glm::value_ptr(this->viewMat));
    GLuint colourLoc = glGetUniformLocation(Object::objectShaderProgram, "colourIn");
    glUniform3f(colourLoc, this->colour[0], this->colour[1], this->colour[2]);

    glBufferData(GL_ARRAY_BUFFER, this->numVertices * sizeof(float) * 2, this->vertices->data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, false, 2 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    glDrawArrays(GL_LINE_STRIP, 0, this->numVertices);
}

void Object::setMass(float mass) {
    this->mass = mass;
}

void Object::setScale(float scale) {
    this->scale = scale;
}

void Object::setColour(float r, float g, float b) {
    this->colour[0] = r;
    this->colour[1] = g;
    this->colour[2] = b;
}

void Object::translate(float x, float y) {
    this->x += x;
    this->y += y;
    this->viewMat = glm::scale(glm::mat4(1.0f), glm::vec3(scaleX, scaleY, 0.0f));
    // this->viewMat *= glm::translate(glm::mat4(1.0f), glm::vec3(this->x * Object::scaleX, this->y * Object::scaleY, 0.0f));
}

void Object::setActive(void) {
    this->active = true;
}

void Object::setInactive(void) {
    this->active = false;
}

void Object::printObject() {
    cout << "Mass: " << this->mass << endl;
    cout << "Scale: " << this->scale << endl;
    cout << "Position: (" << this->x << ", " << this->y << ")" << endl;
    for (int i = 0; i < this->vertices->size(); i++) {
        cout << "Vertex " << i << ": " << this->vertices->at(i).x << ", " << this->vertices->at(i).y << endl;
    }
}

int Object::loadAllEdges(vector<float> *edges) {
    int total = 0;
    for (int i = 0; i < Object::objects.size(); i++) {
        Object *o = Object::objects.at(i);
        if (o->active) {
            for (int j = 0; j < o->numEdges; j++) {
                edges->push_back(o->edges->at(j).x1);
                edges->push_back(o->edges->at(j).y1);
                edges->push_back(o->edges->at(j).x2);
                edges->push_back(o->edges->at(j).y2);
                edges->push_back(o->edges->at(j).nx);
                edges->push_back(o->edges->at(j).ny);
                edges->push_back(o->edges->at(j).elasticity);
                total++;
            }
        }
    }
    return total;
}

void Object::initObjects(int screenWidth, int screenHeight, float scaleFactor, float simulationWidth, float simulationHeight) {
    if (Object::initialized) return;

    Object::screenHeight = screenHeight;
    Object::screenWidth = screenWidth;
    Object::scaleFactor = scaleFactor;
    Object::simulationHeight = simulationHeight;
    Object::simulationWidth = simulationWidth;

    Object::objectShaderProgram = Utils::createShaderProgram("shaders/objectVert.glsl", "shaders/objectFrag.glsl");
    Object::objects = vector<Object *>();

    Object::scaleX = (float)Object::simulationWidth / (float)Object::screenWidth;
    Object::scaleY = (float)Object::simulationHeight / (float)Object::screenHeight;

    Object::initialized = true;
}

void Object::update(void) {
    for (int i = 0; i < Object::objects.size(); i++) {
        if (Object::objects.at(i)->active) Object::objects.at(i)->drawObject();
    }
}