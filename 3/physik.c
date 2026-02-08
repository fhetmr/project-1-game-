#include "physik.h"
#include "cam.h"
#include <math.h>
#include <stdio.h> // Для налагодження, якщо потрібно

// --- Налаштування фізики ---
#define PLAYER_RADIUS 0.3f      // Радіус "тіла" гравця
#define PLAYER_HEIGHT 1.8f      // Повний зріст
#define EYE_LEVEL 1.6f          // Висота камери від п'ят
#define GRAVITY -9.8f           // Реалістична гравітація (потрібно множити на dt)
#define JUMP_FORCE 5.0f         // Сила стрибка (імпульс)

// Глобальний масив об'єктів
Box sceneObjects[256]; 
int objectCount = 0;

// --- Генерація ландшафту ---
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
    // Додаємо трохи "шуму" для деталізації, щоб земля не була надто гладкою
    float h = phys_noise(x * freq, z * freq) * amp;
    h += phys_noise(x * 0.1f, z * 0.1f) * 0.5f;
    return h;
}

// --- Ініціалізація ---
void init_physics(PlayerPhysics* p) {
    p->height = EYE_LEVEL;
    p->verticalVelocity = 0.0f;
    p->gravity = GRAVITY;
    p->jumpPower = JUMP_FORCE;
    p->isGrounded = 0;
    objectCount = 0; 
}

// --- Перевірка колізій (AABB vs Cylinder) ---
// Повертає 1, якщо є зіткнення
int check_collision(float px, float py, float pz, Box b) {
    // 1. Перевірка по висоті (Y)
    // Гравець займає простір від (py - EYE_LEVEL) до (py - EYE_LEVEL + PLAYER_HEIGHT)
    float playerBottom = py - EYE_LEVEL;
    float playerTop = playerBottom + PLAYER_HEIGHT;

    // Якщо гравець повністю вище або повністю нижче боксу - колізії немає
    if (playerBottom >= b.maxY || playerTop <= b.minY) return 0;

    // 2. Перевірка по горизонталі (X, Z) з урахуванням радіусу (товщини)
    // Розширюємо бокс на радіус гравця для перевірки
    // Це емулює циліндр гравця через розширений AABB
    float r = PLAYER_RADIUS;
    if (px + r > b.minX && px - r < b.maxX &&
        pz + r > b.minZ && pz - r < b.maxZ) {
        return 1;
    }

    return 0;
}

// --- Основний цикл фізики ---
// dt - delta time (час у секундах, що пройшов з минулого кадру, наприклад 0.016)
void update_physics(PlayerPhysics* p, float dt) {
    // 1. Застосовуємо гравітацію (V = V0 + a*t)
    p->verticalVelocity += p->gravity * dt;
    
    // Пропонована нова позиція Y
    float nextY = cam.y + p->verticalVelocity * dt;
    
    // --- ПЕРЕВІРКА СТЕЛІ (Head Collision) ---
    // Якщо летимо вгору, перевіряємо, чи не вдарилися об низ об'єкта
    if (p->verticalVelocity > 0) {
        for(int i = 0; i < objectCount; i++) {
            Box b = sceneObjects[i];
            // Перевіряємо, чи ми всередині проекції об'єкта по X/Z
            if (cam.x + PLAYER_RADIUS > b.minX && cam.x - PLAYER_RADIUS < b.maxX &&
                cam.z + PLAYER_RADIUS > b.minZ && cam.z - PLAYER_RADIUS < b.maxZ) {
                
                // Якщо голова (nextY + маківка) перетинає низ боксу
                float headY = nextY - EYE_LEVEL + PLAYER_HEIGHT;
                if (headY > b.minY && (cam.y - EYE_LEVEL + PLAYER_HEIGHT) <= b.minY) {
                    p->verticalVelocity = 0; // Вдарилися головою, зупиняємось
                    nextY = cam.y; // Скасовуємо рух вгору
                    break;
                }
            }
        }
    }

    // --- ПЕРЕВІРКА ПІДЛОГИ (Ground Collision) ---
    float terrainHeight = get_real_terrain_height(cam.x, cam.z);
    float highestGround = terrainHeight;

    // Шукаємо найвищу тверду поверхню під ногами
    for(int i = 0; i < objectCount; i++) {
        Box b = sceneObjects[i];
        
        // Чи ми над об'єктом?
        if (cam.x + PLAYER_RADIUS > b.minX && cam.x - PLAYER_RADIUS < b.maxX &&
            cam.z + PLAYER_RADIUS > b.minZ && cam.z - PLAYER_RADIUS < b.maxZ) {
            
            // Чи були ми над ним у попередньому кадрі? (Щоб не застрибувати миттєво на стіл знизу)
            float playerFeetPrev = cam.y - EYE_LEVEL;
            if (playerFeetPrev >= b.maxY - 0.1f) { // 0.1f толерантність
                if (b.maxY > highestGround) {
                    highestGround = b.maxY;
                }
            }
        }
    }

    // Поточна висота "ніг" гравця
    float playerFeetNext = nextY - EYE_LEVEL;

    // Реакція на землю
    if (playerFeetNext <= highestGround) {
        // Ми впали на землю або під неї
        cam.y = highestGround + EYE_LEVEL; // Виштовхуємо на поверхню
        p->verticalVelocity = 0;
        p->isGrounded = 1;
    } else {
        // Ми в повітрі
        cam.y = nextY;
        p->isGrounded = 0;
    }
}