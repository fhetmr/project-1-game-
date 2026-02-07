#include "physik.h"
#include <math.h>


Box sceneObjects[10]; 
int objectCount = 0;

static float hash(float n) { 
    return fmodf(sinf(n) * 43758.5453123f, 1.0f); 
}

static float interpolate(float a, float b, float x) {
    float ft = x * 3.1415927f;
    float f = (1.0f - cosf(ft)) * 0.5f;
    return a * (1.0f - f) + b * f;
}

static float phys_noise(float x, float z) {
    float ix = floorf(x);
    float iz = floorf(z);
    float fx = x - ix;
    float fz = z - iz;
    float n = ix + iz * 57.0f;
    float a = hash(n);
    float b = hash(n + 1.0f);
    float c = hash(n + 57.0f);
    float d = hash(n + 58.0f);
    float i1 = interpolate(a, b, fx);
    float i2 = interpolate(c, d, fx);
    return interpolate(i1, i2, fz);
}

float get_real_terrain_height(float x, float z) {
    float freq = 0.025f; 
    float amp = 5.0f;
    float h = phys_noise(x * freq, z * freq) * amp;
    h += phys_noise(x * 0.1f, z * 0.1f) * 0.5f;
    return h;
}

void init_physics(PlayerPhysics* p) {
    p->height = 2.2f;
    p->verticalVelocity = 0.0f;
    p->gravity = -0.015f;
    p->jumpPower = 0.28f;
    p->isGrounded = 0;

    objectCount = 1;
    sceneObjects[0] = (Box){-2, 0, -2, 2, 4, 2};
}

void update_physics(PlayerPhysics* p, float terrainScale) {
    p->verticalVelocity += p->gravity;
    cam.y += p->verticalVelocity;

    float terrainY = get_real_terrain_height(cam.x, cam.z);
    float highestPoint = terrainY;


    for(int i = 0; i < objectCount; i++) {
        Box b = sceneObjects[i];

        if (cam.x > b.minX && cam.x < b.maxX && cam.z > b.minZ && cam.z < b.maxZ) {

            if (cam.y >= b.maxY + p->height - 0.5f) {
                if (b.maxY > highestPoint) highestPoint = b.maxY;
            }
        }
    }

    float finalGroundY = highestPoint + p->height;

    if (cam.y <= finalGroundY) {
        cam.y = finalGroundY;
        p->verticalVelocity = 0;
        p->isGrounded = 1;
    } else {
        p->isGrounded = 0;
    }
}
int check_collision(float px, float py, float pz, Box b) {
    float r = 0.5f;
    return (px + r > b.minX && px - r < b.maxX) &&
           (py + 0.5f > b.minY && py - 2.5f < b.maxY) &&
           (pz + r > b.minZ && pz - r < b.maxZ);
}