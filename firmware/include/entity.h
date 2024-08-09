#ifndef __ENTITY_H__
#define __ENTITY_H__

#include "sprite.h"

/*
 * This is a placeholder until the game engine is working, but still
 * allows us to have an interface for the renderer to work
 */
struct entity {
    struct sprite_component sprite;  // Visual component of the entity
};

#endif /* __ENTITY_H__ */
