#include "npc.h"
#include <math.h>
#include <GL/freeglut.h>

// Зовнішня функція для отримання висоти ландшафту
extern float get_real_terrain_height(float x, float z);

NPC create_npc(float x, float z) {
    NPC n;
    n.x = x;
    n.z = z;
    n.angle = 0.0f;
    n.isMoving = 0; // Ініціалізуємо стан спокою
    
    n.physics.height = 1.0f;
    n.physics.verticalVelocity = 0.0f;
    n.physics.gravity = -0.012f;
    n.physics.isGrounded = 0;
    
    n.y = get_real_terrain_height(x, z) + n.physics.height;
    
    float r = 0.75f;
    n.bounds = (Box){x - r, n.y - 1.0f, z - r, x + r, n.y + 1.0f, z + r};
    return n;
}

void update_npc(NPC* n, float playerX, float playerZ, NPC* allNpcs, int npcCount, int myIndex) {
    // 1. Розрахунок дистанції до гравця
    float dx = playerX - n->x;
    float dz = playerZ - n->z;
    float distance = sqrtf(dx * dx + dz * dz);

    // 2. Поворот до гравця
    n->angle = atan2f(dx, dz) * 180.0f / 3.14159f;

    // 3. Логіка ходіння та КОЛІЗІЯ МІЖ NPC
    float speed = 0.05f;          
    float followRadius = 20.0f;
    float stopRadius = 2.5f;
    float separationRadius = 1.8f; // Радіус особистого простору NPC

    float moveX = 0, moveZ = 0;

    // Переслідування гравця
    if (distance < followRadius && distance > stopRadius) {
        moveX += (dx / distance) * speed;
        moveZ += (dz / distance) * speed;
        n->isMoving = 1;
    } else {
        n->isMoving = 0;
    }

    // --- КОЛІЗІЯ З ІНШИМИ NPC (Відштовхування) ---
    for (int i = 0; i < npcCount; i++) {
        if (i == myIndex) continue; // Не перевіряємо себе

        float odx = n->x - allNpcs[i].x;
        float odz = n->z - allNpcs[i].z;
        float distToOther = sqrtf(odx * odx + odz * odz);

        if (distToOther < separationRadius && distToOther > 0.001f) {
            // Сила відштовхування: чим ближче, тим сильніше штовхаємо
            float push = (separationRadius - distToOther) * 0.02f;
            moveX += (odx / distToOther) * push;
            moveZ += (odz / distToOther) * push;
            n->isMoving = 1; // NPC рухається, бо його штовхають
        }
    }

    // Застосовуємо рух
    n->x += moveX;
    n->z += moveZ;

    // 4. Гравітація та ландшафт (твій код без змін)
    n->physics.verticalVelocity += n->physics.gravity;
    n->y += n->physics.verticalVelocity;
    float groundY = get_real_terrain_height(n->x, n->z);
    
    if (n->y < groundY + n->physics.height) {
        n->y = groundY + n->physics.height;
        n->physics.verticalVelocity = 0;
        n->physics.isGrounded = 1;
    } else {
        n->physics.isGrounded = 0;
    }

    // 5. Оновлення Bounding Box
    float r = 0.75f;
    n->bounds = (Box){n->x - r, n->y - 1.0f, n->z - r, 
                      n->x + r, n->y + 1.0f, n->z + r};
}
void draw_npc(NPC* n, GLuint shader) {
    // Параметри анімації
    float time = glutGet(GLUT_ELAPSED_TIME) * 0.008f; 
    float bobbing = 0.0f;
    float tilt = 0.0f;

    if (n->isMoving) {
        // Математика "кроку": вгору-вниз та нахил вбік
        bobbing = fabsf(sinf(time)) * 0.15f; 
        tilt = sinf(time) * 5.0f; 
    }

    glPushMatrix();
    
    // Позиціювання (з урахуванням підстрибування)
    glTranslatef(n->x, n->y + bobbing, n->z);
    glRotatef(n->angle, 0, 1, 0); 
    glRotatef(tilt, 0, 0, 1); // Нахил корпусу при ходьбі

    float s = 0.75f;

    // --- ТІЛО (КУБ) ---
    glBegin(GL_QUADS);
        // Передня грань
        glColor3f(1.0f, 1.0f, 1.0f);
        glVertex3f(-s, -s,  s); glVertex3f( s, -s,  s);
        glVertex3f( s,  s,  s); glVertex3f(-s,  s,  s);

        // Задня грань
        glVertex3f(-s, -s, -s); glVertex3f(-s,  s, -s);
        glVertex3f( s,  s, -s); glVertex3f( s, -s, -s);

        // Верхня грань
        glColor3f(0.9f, 0.9f, 0.9f);
        glVertex3f(-s,  s, -s); glVertex3f(-s,  s,  s);
        glVertex3f( s,  s,  s); glVertex3f( s,  s, -s);

        // Нижня грань
        glVertex3f(-s, -s, -s); glVertex3f( s, -s, -s);
        glVertex3f( s, -s,  s); glVertex3f(-s, -s,  s);

        // Права грань
        glColor3f(0.8f, 0.8f, 0.8f);
        glVertex3f( s, -s, -s); glVertex3f( s,  s, -s);
        glVertex3f( s,  s,  s); glVertex3f( s, -s,  s);

        // Ліва грань
        glVertex3f(-s, -s, -s); glVertex3f(-s, -s,  s);
        glVertex3f(-s,  s,  s); glVertex3f(-s,  s, -s);
    glEnd();

    // --- ОЧІ (ЧОРНИЙ КВАДРАТ) ---
    glColor3f(0.0f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
        glVertex3f(-0.3f, 0.1f, s + 0.01f);
        glVertex3f( 0.3f, 0.1f, s + 0.01f);
        glVertex3f( 0.3f, 0.3f, s + 0.01f);
        glVertex3f(-0.3f, 0.3f, s + 0.01f);
    glEnd();

    glPopMatrix();
}