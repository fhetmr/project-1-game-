#ifndef CAM_H
#define CAM_H

#include <GL/glew.h>

typedef struct {
    float x, y, z;
    float yaw, pitch;
    float frontX, frontY, frontZ;
} Camera;

extern Camera cam;

void update_camera_vectors();
void move_camera(unsigned char key, float speed);
void rotate_camera(int x, int y, int winWidth, int winHeight);

#endif