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


TerrainMesh generate_terrain(int size, float scale);
void free_terrain(TerrainMesh* mesh);

#endif