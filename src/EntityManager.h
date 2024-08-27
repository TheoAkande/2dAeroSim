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

#ifndef ENTITYMANAGER_H
#define ENTITYMANAGER_H

class Entity
{
    private:
        bool visible;
        virtual void draw(void);

        static vector<Entity *> entities;
    public:
        Entity(void);
        void setVisible(void);
        void setInvisible(void);
        bool isVisible(void);

        static void drawEntities(void);
    friend class Updateable;
};

class Updateable : public Entity
{
    private:
        bool active;
        virtual void update(bool click, int mouseX, int mouseY);

        static vector<Updateable *> updateables;
    public:
        Updateable(void);
        void setActive(void);
        void setInactive(void);
        bool isActive(void);

        static void updateEntities(bool click, int mouseX, int mouseY);
};

#endif