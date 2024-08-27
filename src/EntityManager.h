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

using namespace std;

class Entity
{
    private:
        bool active;
        virtual void draw(void);

        static vector<Entity *> entities;
    public:
        Entity(void) { 
            this->active = true; 
            entities.push_back(this);
        }
        void setActive(void) { this->active = true; }
        void setInactive(void) { this->active = false; }

        static void drawEntities(void) {
            for (Entity *entity : entities) {
                if (entity->active) {
                    entity->draw();
                }
            }
        }
    friend class Updateable;
};

class Updateable : public Entity
{
    private:
        virtual void update(bool click, int mouseX, int mouseY);

        static vector<Updateable *> updateables;
    public:
        Updateable(void) : Entity() {
            updateables.push_back(this);
        }

        static void updateEntities(bool click, int mouseX, int mouseY) {
            for (Updateable *updateable : updateables) {
                if (updateable->active) {
                    updateable->update(click, mouseX, mouseY);
                }
            }
        }
};