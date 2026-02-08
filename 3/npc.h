#ifndef NPC_H
#define NPC_H

#include <GL/glew.h>
#include "physik.h"

typedef struct {
    float x, y, z;
    float angle;
    int isMoving; 
    float hoverTimer;
PlayerPhysics physics;
    Box bounds;
    int id;
} NPC;


NPC create_npc(float x, float z);

// Додай масив усіх NPC та їх кількість у параметри
void update_npc(NPC* n, float playerX, float playerZ, NPC* allNpcs, int npcCount, int myIndex);

void draw_npc(NPC* n, GLuint shader);

#endif