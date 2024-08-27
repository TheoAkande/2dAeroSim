#include "EntityManager.h"

using namespace std;

vector<Entity *> Entity::entities;

vector<Updateable *> Updateable::updateables;

Entity::Entity(void) { 
    this->visible = true; 
    entities.push_back(this);
}

void Entity::setVisible(void) { this->visible = true; }
void Entity::setInvisible(void) { this->visible = false; }
bool Entity::isVisible(void) { return this->visible; }

void Entity::draw(void) { }

void Entity::drawEntities(void) {
    for (Entity *entity : entities) {
        if (entity->visible) {
            entity->draw();
        }
    }
}


Updateable::Updateable(void) : Entity() {
    this->active = true;
    updateables.push_back(this);
}

void Updateable::update(bool click, int mouseX, int mouseY) { }
void Updateable::setActive(void) { this->active = true; }
void Updateable::setInactive(void) { this->active = false; }
bool Updateable::isActive(void) { return this->active; }

void Updateable::updateEntities(bool click, int mouseX, int mouseY) {
    for (Updateable *updateable : updateables) {
        if (updateable->active) {
            updateable->update(click, mouseX, mouseY);
        }
    }
}