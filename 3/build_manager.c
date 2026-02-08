#include "build_manager.h"
#include "cam.h"           
#include <GL/glew.h>       
#include <math.h>          
#include <stdio.h>
#include <stdbool.h>      

// Зовнішні змінні з main.c та інших файлів
extern float get_real_terrain_height(float x, float z);
extern Model wallModel;
extern Model doorModel;

int buildMode = 0;
Model* buildSelection = NULL;
float buildRotation = 0.0f;
float buildDistance = 8.0f;  
StaticObject buildings[500]; // MAX_BUILDINGS
int buildingCount = 0;

// --- ДОПОМІЖНА ФУНКЦІЯ: РОЗРАХУНОК ПОВЕРНУТОЇ КОЛІЗІЇ ---
// Ця функція міняє ширину і глибину місцями, якщо об'єкт повернуто на 90/270 градусів
Box calculate_rotated_bounds(Model* model, float x, float y, float z, float rot, float scale) {
    Box b = model->bounds;
    
    // Рахуємо центр моделі для коректного зміщення
    float ox = (b.minX + b.maxX) / 2.0f;
    float oz = (b.minZ + b.maxZ) / 2.0f;

    // Рахуємо "радіуси" (половину розміру) по осях
    float rx = (b.maxX - b.minX) * 0.5f * scale;
    float rz = (b.maxZ - b.minZ) * 0.5f * scale;
    float ry = (b.maxY - b.minY) * scale;

    // Перевіряємо кут повороту (наближено до 90 градусів)
    int angle = (int)fabsf(roundf(rot / 90.0f)) % 2;

    if (angle != 0) { // Якщо повернуто боком (90, 270, -90...)
        // Свопаємо (міняємо місцями) X та Z радіуси
        return (Box){ x - rz, y, z - rx, x + rz, y + ry, z + rx };
    } else {
        // Стандартна орієнтація (0, 180, 360...)
        return (Box){ x - rx, y, z - rz, x + rx, y + ry, z + rz };
    }
}

// Функція перевірки зіткнення двох AABB
bool check_aabb_collision(Box a, Box b) {
    return (a.minX <= b.maxX && a.maxX >= b.minX) &&
           (a.minY <= b.maxY && a.maxY >= b.minY) &&
           (a.minZ <= b.maxZ && a.maxZ >= b.minZ);
}

void update_building_logic(bool clicked) {
    if (!buildMode || !buildSelection) return;

    // 1. Позиція на сітці
    float targetX = cam.x + cam.frontX * buildDistance;
    float targetZ = cam.z + cam.frontZ * buildDistance;

    float gridSize = 4.0f;
    float gx = roundf(targetX / gridSize) * gridSize;
    float gz = roundf(targetZ / gridSize) * gridSize;
    float gy = get_real_terrain_height(gx, gz);

    // 2. Розрахунок колізії
    Box previewBox = calculate_rotated_bounds(buildSelection, gx, gy, gz, buildRotation, 1.5f);

    // 3. Перевірка колізії (залишаємо як було)
    bool isColliding = false;
    for (int i = 0; i < buildingCount; i++) {
        if (check_aabb_collision(previewBox, buildings[i].worldBounds)) { isColliding = true; break; }
    }
    
    // Блок малювання (glPushMatrix...glPopMatrix) ТУТ БІЛЬШЕ НЕ ПОТРІБЕН
    // Рендерер сам намалює привида, використовуючи SceneData

    // 5. Встановлення
    if (clicked && !isColliding && buildingCount < 500) {
        buildings[buildingCount].model = buildSelection;
        buildings[buildingCount].x = gx;
        buildings[buildingCount].y = gy;
        buildings[buildingCount].z = gz;
        buildings[buildingCount].scale = 1.5f;
        buildings[buildingCount].rotation = buildRotation;
        buildings[buildingCount].worldBounds = previewBox;
        buildingCount++;
    }
}

// --- ЗБЕРЕЖЕННЯ ТА ЗАВАНТАЖЕННЯ ---

void save_buildings(const char* filename) {
    FILE* f = fopen(filename, "w");
    if (!f) return;

    for (int i = 0; i < buildingCount; i++) {
        int modelID = (buildings[i].model == &doorModel) ? 1 : 0;
        fprintf(f, "%d %f %f %f %f %f\n", 
                modelID, buildings[i].x, buildings[i].y, buildings[i].z, 
                buildings[i].rotation, buildings[i].scale);
    }
    fclose(f);
    printf("Світ збережено (%d об'єктів)\n", buildingCount);
}

void load_buildings(const char* filename) {
    FILE* f = fopen(filename, "r");
    if (!f) return;

    buildingCount = 0;
    int modelID;
    float x, y, z, rot, scl;

    while (fscanf(f, "%d %f %f %f %f %f", &modelID, &x, &y, &z, &rot, &scl) == 6) {
        if (buildingCount >= 500) break;

        buildings[buildingCount].model = (modelID == 1) ? &doorModel : &wallModel;
        buildings[buildingCount].x = x;
        buildings[buildingCount].y = y;
        buildings[buildingCount].z = z;
        buildings[buildingCount].rotation = rot;
        buildings[buildingCount].scale = scl;

        // Перераховуємо повернуту колізію при завантаженні
        buildings[buildingCount].worldBounds = calculate_rotated_bounds(
            buildings[buildingCount].model, x, y, z, rot, scl
        );

        buildingCount++;
    }
    fclose(f);
    printf("Світ завантажено (%d об'єктів)\n", buildingCount);
}