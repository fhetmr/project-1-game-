#include "model_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Model load_model(const char* filename) {
    Model m = {0};
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Error: Could not open model %s\n", filename);
        return m;
    }

    float *temp_v = malloc(sizeof(float) * 600000); 
    float *out_v = malloc(sizeof(float) * 1800000); // Збільшено для тріангуляції
    int v_idx = 0, out_idx = 0;

    float minX = 1e6, minY = 1e6, minZ = 1e6;
    float maxX = -1e6, maxY = -1e6, maxZ = -1e6;

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        if (line[0] == 'v' && line[1] == ' ') {
            float x, y, z;
            sscanf(line, "v %f %f %f", &x, &y, &z);
            temp_v[v_idx++] = x; temp_v[v_idx++] = y; temp_v[v_idx++] = z;
            if(x < minX) minX = x; if(x > maxX) maxX = x;
            if(y < minY) minY = y; if(y > maxY) maxY = y;
            if(z < minZ) minZ = z; if(z > maxZ) maxZ = z;
        } 
        else if (line[0] == 'f' && line[1] == ' ') {
            int v_ind[4];
            int count = 0;
            char* ptr = line + 2;

            // Зчитуємо всі індекси вершин у рядку (їх може бути 3 або 4)
            while (*ptr && count < 4) {
                v_ind[count++] = atoi(ptr);
                while (*ptr && *ptr != ' ') ptr++; // Пропускаємо v/vt/vn
                while (*ptr == ' ') ptr++;        // Переходимо до наступного блоку
            }

            // Якщо це трикутник (1-2-3)
            if (count >= 3) {
                for (int i = 0; i < 3; i++) {
                    int idx = (v_ind[i] - 1) * 3;
                    out_v[out_idx++] = temp_v[idx];
                    out_v[out_idx++] = temp_v[idx+1];
                    out_v[out_idx++] = temp_v[idx+2];
                }
            }
            // Якщо це чотирикутник, розбиваємо на два трикутники (1-2-3 та 1-3-4)
            if (count == 4) {
                int tri2[3] = {v_ind[0], v_ind[2], v_ind[3]};
                for (int i = 0; i < 3; i++) {
                    int idx = (tri2[i] - 1) * 3;
                    out_v[out_idx++] = temp_v[idx];
                    out_v[out_idx++] = temp_v[idx+1];
                    out_v[out_idx++] = temp_v[idx+2];
                }
            }
        }
    }

    m.vertexCount = out_idx / 3;
    m.bounds = (Box){minX, minY, minZ, maxX, maxY, maxZ};

    if (m.vertexCount > 0) {
        glGenVertexArrays(1, &m.vao);
        glGenBuffers(1, &m.vbo);
        glBindVertexArray(m.vao);
        glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
        glBufferData(GL_ARRAY_BUFFER, out_idx * sizeof(float), out_v, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
    }

    free(temp_v); free(out_v);
    fclose(file);
    return m;
}

// Функція draw_model залишається без змін (як у минулій відповіді)
void draw_model(Model* m, float x, float y, float z, float scale) {
    if (m == NULL || m->vertexCount <= 0 || m->vao == 0) return;
    glPushMatrix();
    glTranslatef(x, y, z);
    glScalef(scale, scale, scale);
    glBindVertexArray(m->vao); 
    glDrawArrays(GL_TRIANGLES, 0, m->vertexCount);
    glBindVertexArray(0);
    glPopMatrix();
}