#ifndef RENDERER_H
#define RENDERER_H

#include <GL/glew.h>
#include <GL/freeglut.h>
#include "build_manager.h" // Тут має бути оголошено StaticObject
#include "gen.h"
#include "cam.h"
#include "npc.h"

// Структура для передачі даних з main в renderer
typedef struct {

    Chunk* chunks;   // Масив чанків
    int numChunks;   // Кількість чанків (зазвичай 9)
    float dayTime;
    GLuint grassTexture;
    StaticObject* statics;
    int staticCount;
    StaticObject* buildings;
    int buildCount;
    NPC* npcs;
    int npcCount;
    int buildMode;
    Model* buildSelection;
    float buildX, buildY, buildZ;
    float buildRotation;
    bool isColliding;
} SceneData;

typedef struct {
    GLuint skyShader, terrainShader;
    GLuint skyVAO, skyVBO;
    GLint windDirLoc, windPowerLoc, windGustLoc, viewPosLoc, timeLoc, sunDirLoc;
} RenderState;

// Оголошуємо функції
void init_renderer(RenderState* rs);
void render_scene(RenderState* rs, SceneData* data);

#endif