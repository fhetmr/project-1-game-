#ifndef PHYSIK_H
#define PHYSIK_H

#include "cam.h"

typedef struct {
    float minX, minY, minZ;
    float maxX, maxY, maxZ;
} Box;

typedef struct {
    float height;
    float verticalVelocity;
    float gravity;
    float jumpPower;
    int isGrounded;
} PlayerPhysics;

void init_physics(PlayerPhysics* p);
void update_physics(PlayerPhysics* p, float terrainScale);
int check_collision(float px, float py, float pz, Box b);


extern Box sceneObjects[10]; 
extern int objectCount;

#endif