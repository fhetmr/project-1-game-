#ifndef GEN_H
#define GEN_H

#include <GL/glew.h>

typedef struct {
    float x, y, z;
    float nx, ny, nz;
    float u, v;
} Vertex;

typedef struct {
    GLuint vao, vbo, ebo;
    int indexCount;
} TerrainMesh;

typedef struct {
    TerrainMesh mesh;  // вершини та індекси чанку
    float x;           // світова позиція центру чанку X
    float z;           // світова позиція центру чанку Z
    float scale;       // масштаб чанку
    int size;          // розмір чанку (кількість вершин по стороні - 1)
    int active;        // чи рендерити
} Chunk;


TerrainMesh generate_terrain_chunk(int size, float scale, float offsetX, float offsetZ);
void free_terrain(TerrainMesh* mesh);
float get_height(float x, float z);

#endif