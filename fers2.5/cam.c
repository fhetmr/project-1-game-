#include "cam.h"
#include <math.h>

Camera cam = {0.0f, 10.0f, 30.0f, -90.0f, 0.0f};

void update_camera_vectors() {
    float radYaw = cam.yaw * M_PI / 180.0f;
    float radPitch = cam.pitch * M_PI / 180.0f;

    cam.frontX = cosf(radYaw) * cosf(radPitch);
    cam.frontY = sinf(radPitch);
    cam.frontZ = sinf(radYaw) * cosf(radPitch);
}

void move_camera(unsigned char key, float speed) {
    if (key == 'w') { cam.x += cam.frontX * speed; cam.y += cam.frontY * speed; cam.z += cam.frontZ * speed; }
    if (key == 's') { cam.x -= cam.frontX * speed; cam.y -= cam.frontY * speed; cam.z -= cam.frontZ * speed; }

    float rightX = -cam.frontZ; 
    float rightZ = cam.frontX;
    if (key == 'a') { cam.x -= rightX * speed; cam.z -= rightZ * speed; }
    if (key == 'd') { cam.x += rightX * speed; cam.z += rightZ * speed; }
}

void rotate_camera(float dx, float dy) {
    float sensitivity = 0.15f; // Налаштуй чутливість під себе
    
    // dx та dy — це вже готова різниця (x - cx) з main.c
    cam.yaw   += dx * sensitivity;
    cam.pitch -= dy * sensitivity; // Інвертуємо, щоб мишка вгору = погляд вгору

    // Обмеження вертикального огляду (щоб не "зламати" шию)
    if (cam.pitch > 89.0f)  cam.pitch = 89.0f;
    if (cam.pitch < -89.0f) cam.pitch = -89.0f;

    // Перераховуємо вектори напрямку (сферичні координати)
    float yawRad   = cam.yaw   * (M_PI / 180.0f);
    float pitchRad = cam.pitch * (M_PI / 180.0f);

    cam.frontX = cosf(yawRad) * cosf(pitchRad);
    cam.frontY = sinf(pitchRad);
    cam.frontZ = sinf(yawRad) * cosf(pitchRad);
}