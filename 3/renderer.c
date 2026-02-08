#include "renderer.h"
#include "ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

RenderState rs;

// Внутрішні змінні часу для рендерера
static int lastTime = 0;
static float dayTime = 0.0f;

// --- Допоміжні функції ---

static char* read_shader_src(const char* p) {
    FILE* f = fopen(p, "rb");
    if(!f) return NULL;
    fseek(f, 0, SEEK_END); long l = ftell(f); fseek(f, 0, SEEK_SET);
    char* b = malloc(l + 1); fread(b, 1, l, f); b[l] = 0;
    fclose(f);
    return b;
}

static void check_compile_errors(GLuint shader, const char* type) {
    GLint success;
    char infoLog[1024];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 1024, NULL, infoLog);
        printf("ERROR::SHADER_COMPILATION_ERROR of type: %s\n%s\n", type, infoLog);
    }
}

static GLuint create_prog_internal(const char *vP, const char *fP) {
    char *vS = read_shader_src(vP), *fS = read_shader_src(fP);
    if (!vS || !fS) {
        printf("CRITICAL ERROR: Shader files missing: %s or %s\n", vP, fP);
        return 0;
    }
    GLuint v = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(v, 1, (const char**)&vS, 0);
    glCompileShader(v);
    check_compile_errors(v, "VERTEX");

    GLuint f = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(f, 1, (const char**)&fS, 0);
    glCompileShader(f);
    check_compile_errors(f, "FRAGMENT");

    GLuint p = glCreateProgram();
    glAttachShader(p, v);
    glAttachShader(p, f);
    glLinkProgram(p);
    
    free(vS); free(fS);
    return p;
}

// --- Основний інтерфейс ---

void init_renderer(RenderState* state) {
    state->skyShader = create_prog_internal("sky.vs", "sky.fs");
    state->terrainShader = create_prog_internal("terrain.vs", "terrain.fs");
    
    // Отримуємо локації юніформів для ландшафту
    state->windDirLoc   = glGetUniformLocation(state->terrainShader, "u_windDir");
    state->windPowerLoc = glGetUniformLocation(state->terrainShader, "u_windPower");
    state->windGustLoc  = glGetUniformLocation(state->terrainShader, "u_windGust");
    state->viewPosLoc   = glGetUniformLocation(state->terrainShader, "viewPos");
    state->timeLoc      = glGetUniformLocation(state->terrainShader, "u_time");
    state->sunDirLoc    = glGetUniformLocation(state->terrainShader, "u_sunDir");

    // Skybox VAO (Прямокутник на весь екран)
    float skyV[] = { -1,1,0, -1,-1,0, 1,-1,0, -1,1,0, 1,-1,0, 1,1,0 };
    glGenVertexArrays(1, &state->skyVAO);
    glGenBuffers(1, &state->skyVBO);
    glBindVertexArray(state->skyVAO);
    glBindBuffer(GL_ARRAY_BUFFER, state->skyVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyV), skyV, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    
    printf("Renderer initialized. Shaders: Sky[%u] Terrain[%u]\n", state->skyShader, state->terrainShader);
}

void render_scene(RenderState* state, SceneData* data) {
    // =====================
    // 1. Оновлення часу
    // =====================
    int ticks = glutGet(GLUT_ELAPSED_TIME);
    float dt = (ticks - lastTime) / 1000.0f;
    if (dt > 0.05f) dt = 0.05f;
    lastTime = ticks;
    dayTime += 0.01f * dt;

    float sunX = 0.0f, sunY = sinf(dayTime), sunZ = cosf(dayTime);
    int w = glutGet(GLUT_WINDOW_WIDTH), h = glutGet(GLUT_WINDOW_HEIGHT);

    float view[16], proj[16], identity[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // =====================
    // 2. Проекція
    // =====================
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluPerspective(60.0, (double)w/(h?h:1), 0.1, 1000.0);
    glGetFloatv(GL_PROJECTION_MATRIX, proj);

    // =====================
    // 3. Рендер неба
    // =====================
    glDisable(GL_DEPTH_TEST);
    glUseProgram(state->skyShader);

    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    gluLookAt(0, 0, 0, cam.frontX, cam.frontY, cam.frontZ, 0, 1, 0);
    glGetFloatv(GL_MODELVIEW_MATRIX, view);

    float viewRot[16]; memcpy(viewRot, view, sizeof(float)*16);
    viewRot[12] = viewRot[13] = viewRot[14] = 0.0f;

    glUniformMatrix4fv(glGetUniformLocation(state->skyShader, "view"), 1, GL_FALSE, viewRot);
    glUniformMatrix4fv(glGetUniformLocation(state->skyShader, "projection"), 1, GL_FALSE, proj);
    glUniform3f(glGetUniformLocation(state->skyShader, "u_sunDir"), sunX, sunY, sunZ);
    glUniform3f(glGetUniformLocation(state->skyShader, "u_cameraPos"), cam.x, cam.y, cam.z);
    glUniform2f(glGetUniformLocation(state->skyShader, "u_res"), (float)w, (float)h);
    glUniform1f(glGetUniformLocation(state->skyShader, "u_time"), ticks / 1000.0f);

    glBindVertexArray(state->skyVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glEnable(GL_DEPTH_TEST);

    // =====================
    // 4. Рендер чанків з краєвим туманом
    // =====================
    glUseProgram(state->terrainShader);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    gluLookAt(cam.x, cam.y, cam.z, cam.x + cam.frontX, cam.y + cam.frontY, cam.z + cam.frontZ, 0, 1, 0);
    glGetFloatv(GL_MODELVIEW_MATRIX, view);

    glUniformMatrix4fv(glGetUniformLocation(state->terrainShader, "model"), 1, GL_FALSE, identity);
    glUniformMatrix4fv(glGetUniformLocation(state->terrainShader, "view"), 1, GL_FALSE, view);
    glUniformMatrix4fv(glGetUniformLocation(state->terrainShader, "projection"), 1, GL_FALSE, proj);

    glUniform3f(state->sunDirLoc, sunX, sunY, sunZ);
    glUniform1f(state->timeLoc, ticks / 1000.0f);
    glUniform2f(state->windDirLoc, 1.0f, 0.3f);
    glUniform1f(state->windPowerLoc, 1.0f);
    glUniform1f(state->windGustLoc, 0.7f);

    // Текстура трави
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, data->grassTexture);
    glUniform1i(glGetUniformLocation(state->terrainShader, "grassTex"), 0);

for (int i = 0; i < data->numChunks; i++) {
    if (!data->chunks[i].active) continue;

    float chunkX = data->chunks[i].x;
    float chunkZ = data->chunks[i].z;
    float halfSize = (data->chunks[i].size * data->chunks[i].scale) / 2.0f;

    glUniform2f(glGetUniformLocation(state->terrainShader, "chunkCenterXZ"), chunkX, chunkZ);
    glUniform1f(glGetUniformLocation(state->terrainShader, "chunkHalfSize"), halfSize);

    glBindVertexArray(data->chunks[i].mesh.vao);
    glDrawElements(GL_TRIANGLES, data->chunks[i].mesh.indexCount, GL_UNSIGNED_INT, 0);
}
    glBindVertexArray(0);
    glUseProgram(0);

    // =====================
    // 5. Статичні об’єкти і будівлі (Fixed Pipeline)
    // =====================
    for (int i = 0; i < data->staticCount; i++) {
        draw_model(data->statics[i].model, data->statics[i].x, data->statics[i].y, data->statics[i].z, data->statics[i].scale);
    }

    for (int i = 0; i < data->buildCount; i++) {
        glPushMatrix();
        glTranslatef(data->buildings[i].x, data->buildings[i].y, data->buildings[i].z);
        glRotatef(data->buildings[i].rotation, 0, 1, 0);
        glScalef(data->buildings[i].scale, data->buildings[i].scale, data->buildings[i].scale);

        float ox = (data->buildings[i].model->bounds.minX + data->buildings[i].model->bounds.maxX) / 2.0f;
        float oz = (data->buildings[i].model->bounds.minZ + data->buildings[i].model->bounds.maxZ) / 2.0f;
        glTranslatef(-ox, 0, -oz);

        glBindVertexArray(data->buildings[i].model->vao);
        glDrawArrays(GL_TRIANGLES, 0, data->buildings[i].model->vertexCount);
        glBindVertexArray(0);
        glPopMatrix();
    }

    // =====================
    // 6. Привид будівництва
    // =====================
    if (data->buildMode && data->buildSelection) {
        glPushMatrix();
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(1, 1, 1, 0.5f);

        glTranslatef(data->buildX, data->buildY, data->buildZ);
        glRotatef(data->buildRotation, 0, 1, 0);

        float ox = (data->buildSelection->bounds.minX + data->buildSelection->bounds.maxX) / 2.0f;
        float oz = (data->buildSelection->bounds.minZ + data->buildSelection->bounds.maxZ) / 2.0f;
        glTranslatef(-ox, 0, -oz);

        glBindVertexArray(data->buildSelection->vao);
        glDrawArrays(GL_TRIANGLES, 0, data->buildSelection->vertexCount);
        glBindVertexArray(0);

        glColor4f(1,1,1,1);
        glDisable(GL_BLEND);
        glPopMatrix();
    }

    // =====================
    // 7. UI
    // =====================
    draw_ui(w, h, cam.x, cam.z, 0.0f, data->npcs, data->npcCount, NULL);

    glutSwapBuffers();
}
