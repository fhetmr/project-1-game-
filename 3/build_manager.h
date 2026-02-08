#ifndef BUILD_MANAGER_H
#define BUILD_MANAGER_H

#include "model_manager.h"
#include <stdbool.h>

// 1. Оголошуємо структуру тільки якщо вона ще не була оголошена
// Найкраще винести її в окремий types.h, але поки зробимо так:
#ifndef STATIC_OBJECT_TYPE
#define STATIC_OBJECT_TYPE
typedef struct {
    float x, y, z, scale, rotation;
    Model* model;
    Box worldBounds;
} StaticObject; 
#endif

#define MAX_BUILDINGS 500

// 2. Зовнішні змінні (extern)
extern int buildMode;
extern Model* buildSelection;
extern float buildRotation;
extern StaticObject buildings[MAX_BUILDINGS];
extern int buildingCount;
extern float buildDistance;

// 3. ДОДАЄМО ПРОТОТИПИ (це вирішить помилки "implicit declaration" у main.c)
Box calculate_rotated_bounds(Model* model, float x, float y, float z, float rot, float scale);
bool check_aabb_collision(Box a, Box b);
void update_building_logic(bool clicked);

void save_buildings(const char* filename);
void load_buildings(const char* filename);

#endif