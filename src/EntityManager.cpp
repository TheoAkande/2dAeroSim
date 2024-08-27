#include "EntityManager.h"

using namespace std;

vector<Entity *> Entity::entities;

vector<Updateable *> Updateable::updateables;

Entity::Entity(void) { 
    this->active = true; 
    entities.push_back(this);
}

void Entity::setActive(void) { this->active = true; }
void Entity::setInactive(void) { this->active = false; }
void Entity::draw(void) { }
bool Entity::isActive(void) { return this->active; }

void Entity::drawEntities(void) {
    for (Entity *entity : entities) {
        if (entity->active) {
            entity->draw();
        }
    }
}


Updateable::Updateable(void) : Entity() {
    updateables.push_back(this);
}
void Updateable::update(bool click, int mouseX, int mouseY) { }

void Updateable::updateEntities(bool click, int mouseX, int mouseY) {
    for (Updateable *updateable : updateables) {
        if (updateable->active) {
            updateable->update(click, mouseX, mouseY);
        }
    }
}