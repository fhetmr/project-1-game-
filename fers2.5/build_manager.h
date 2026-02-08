#ifndef BUILD_MANAGER_H
#define BUILD_MANAGER_H

#include "model_manager.h"
#include <stdbool.h>

// Використовуємо StaticObject всюди
typedef struct {
    float x, y, z, scale, rotation;
    Model* model;
    Box worldBounds;
} StaticObject; 

#define MAX_BUILDINGS 500

extern int buildMode;
extern Model* buildSelection;
extern float buildRotation;
extern StaticObject buildings[MAX_BUILDINGS]; // Змінено тип тут
extern int buildingCount;
extern float buildDistance; // Додай це в .h файл

void save_buildings(const char* filename);
void load_buildings(const char* filename);

void update_building_logic(bool clicked);

#endif