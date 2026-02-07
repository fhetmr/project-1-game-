#ifndef NPC_H
#define NPC_H

#include <GL/glew.h>
#include "physik.h"

typedef struct {
    float x, y, z;
    float angle;
    float hoverTimer;
PlayerPhysics physics;
    Box bounds;
    int id;
} NPC;


NPC create_npc(float x, float z);

void update_npc(NPC* n, float playerX, float playerZ);

void draw_npc(NPC* n, GLuint shader);

#endif