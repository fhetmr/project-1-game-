#include "gen.h"
#include <math.h>
#include <stdlib.h>
#include <GL/glew.h>


float hash(float n) { 
    return fmodf(sinf(n) * 43758.5453123f, 1.0f); 
}

float interpolate(float a, float b, float x) {
    float ft = x * 3.1415927f;
    float f = (1.0f - cosf(ft)) * 0.5f;
    return a * (1.0f - f) + b * f;
}

float noise(float x, float z) {
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


float get_height(float x, float z) {

    float freq = 0.025f; 
    float amp = 5.0f;
    
    float h = noise(x * freq, z * freq) * amp;
    
    h += noise(x * 0.1f, z * 0.1f) * 0.5f;
    
    return h;
}

void normalize_vec(float* x, float* y, float* z) {
    float len = sqrtf((*x)*(*x) + (*y)*(*y) + (*z)*(*z));
    if (len > 0) { *x /= len; *y /= len; *z /= len; }
}

TerrainMesh generate_terrain(int size, float scale) {
    int vertexCount = (size + 1) * (size + 1);
    Vertex* vertices = malloc(vertexCount * sizeof(Vertex));
    
    for (int z = 0; z <= size; z++) {
        for (int x = 0; x <= size; x++) {
            int i = z * (size + 1) + x;
            float vx = (x - size / 2.0f) * scale;
            float vz = (z - size / 2.0f) * scale;
            float vy = get_height(vx, vz);
            
            float eps = 0.5f;
            float nx = get_height(vx - eps, vz) - get_height(vx + eps, vz);
            float ny = 2.0f * eps;
            float nz = get_height(vx, vz - eps) - get_height(vx, vz + eps);
            normalize_vec(&nx, &ny, &nz);

            float u = vx * 0.2f;
            float v = vz * 0.2f;

            vertices[i] = (Vertex){vx, vy, vz, nx, ny, nz, u, v};
        }
    }

    int indexCount = size * size * 6;
    unsigned int* indices = malloc(indexCount * sizeof(unsigned int));
    int pointer = 0;
    for (int z = 0; z < size; z++) {
        for (int x = 0; x < size; x++) {
            int topLeft = z * (size + 1) + x;
            int topRight = topLeft + 1;
            int bottomLeft = (z + 1) * (size + 1) + x;
            int bottomRight = bottomLeft + 1;
            
            indices[pointer++] = topLeft;
            indices[pointer++] = bottomLeft;
            indices[pointer++] = topRight;
            indices[pointer++] = topRight;
            indices[pointer++] = bottomLeft;
            indices[pointer++] = bottomRight;
        }
    }

    TerrainMesh mesh;
    mesh.indexCount = indexCount;
    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo);
    glGenBuffers(1, &mesh.ebo);

    glBindVertexArray(mesh.vao);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(Vertex), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(unsigned int), indices, GL_STATIC_DRAW);

    // Атрибуты (Position, Normal, UV)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    free(vertices);
    free(indices);
    return mesh;
}

void free_terrain(TerrainMesh* mesh) {
    glDeleteVertexArrays(1, &mesh->vao);
    glDeleteBuffers(1, &mesh->vbo);
    glDeleteBuffers(1, &mesh->ebo);
}