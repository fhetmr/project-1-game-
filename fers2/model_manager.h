#ifndef MODEL_MANAGER_H
#define MODEL_MANAGER_H

#include <GL/glew.h>
#include "physik.h" // Для типу Box

typedef struct {
    GLuint vao, vbo;
    int vertexCount;
    Box bounds;     // Автоматично розрахована колізія
} Model;

// Завантажує .obj файл та повертає структуру моделі
Model load_model(const char* filename);

// Малює модель у вказаних координатах
void draw_model(Model* m, float x, float y, float z, float scale);

#endif