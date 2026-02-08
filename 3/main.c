#include <GL/glew.h>
#include <GL/freeglut.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "gen.h"
#include "cam.h"
#include "physik.h"
#include "npc.h"
#include "ui.h"
#include "model_manager.h"
#include "build_manager.h"
#include "renderer.h"

// --- Константи світу ---
#define CHUNK_SIZE 100
#define NUM_CHUNKS 9



// --- Зовнішні дані (з build_manager.c та інших) ---
extern int buildMode;
extern Model* buildSelection;
extern float buildRotation;
extern StaticObject buildings[500]; // Має збігатися з MAX_BUILDINGS
extern int buildingCount;
extern float buildDistance;        // ДОДАЙ ЦЕЙ РЯДОК
extern RenderState rs; // Оголошена в renderer.c

extern Box sceneObjects[256];
extern int objectCount;
extern float get_real_terrain_height(float x, float z);

// --- Глобальні змінні ---
GLuint skyShader, terrainShader;
GLuint skyVAO, skyVBO;
GLuint grassTexture;
TerrainMesh terrain;

GLuint windDirLoc;
GLuint windPowerLoc;
GLuint windGustLoc;
GLuint viewPosLoc;


Chunk worldChunks[NUM_CHUNKS];
int currentCenterChunkX = 9999, currentCenterChunkZ = 9999;

int isShiftPressed = 0;
Model houseModel, treeModel, wallModel, doorModel;

#define MAX_STATIC_OBJECTS 100
StaticObject staticObjects[MAX_STATIC_OBJECTS];
int staticObjectCount = 0;

#define MAX_NPCS 50
NPC npcs[MAX_NPCS];
int npcCount = 0;

int keys[256] = {0};
float dayTime = 1.5f;
PlayerPhysics player;

int isTyping = 0;
char inputBuffer[64];
int inputPos = 0;

int physicsEnabled = 2;      
float playerSpeed = 15.0f;
int lastTime = 0;

int targetPX = 0, targetPZ = 0;
int isUpdatingWorld = 0;
int currentUpdateIdx = 0;

void update_world_chunks(float playerX, float playerZ) {
    int pX = (int)roundf(playerX / CHUNK_SIZE);
    int pZ = (int)roundf(playerZ / CHUNK_SIZE);

    // Якщо гравець перейшов межу, і ми ще не в процесі оновлення
    if ((pX != currentCenterChunkX || pZ != currentCenterChunkZ) && !isUpdatingWorld) {
        targetPX = pX;
        targetPZ = pZ;
        isUpdatingWorld = 1;
        currentUpdateIdx = 0; // Починаємо з першого чанку (з 9-ти)
    }

    // Якщо запущено процес оновлення — робимо ТІЛЬКИ ОДИН чанк за виклик (за один кадр)
    if (isUpdatingWorld) {
        int i = (currentUpdateIdx / 3) - 1; // Перетворюємо 0-8 в координати -1, 0, 1
        int j = (currentUpdateIdx % 3) - 1;

        if (worldChunks[currentUpdateIdx].active) {
            free_terrain(&worldChunks[currentUpdateIdx].mesh);
        }

        float offX = (targetPX + i) * CHUNK_SIZE;
        float offZ = (targetPZ + j) * CHUNK_SIZE;

        worldChunks[currentUpdateIdx].mesh = generate_terrain_chunk(CHUNK_SIZE, 1.0f, offX, offZ);
        worldChunks[currentUpdateIdx].x = offX;
        worldChunks[currentUpdateIdx].z = offZ;
        worldChunks[currentUpdateIdx].active = 1;

        currentUpdateIdx++;

        // Коли зробили всі 9 чанків
        if (currentUpdateIdx >= NUM_CHUNKS) {
            isUpdatingWorld = 0;
            currentCenterChunkX = targetPX;
            currentCenterChunkZ = targetPZ;
            printf("World update finished smoothly.\n");
        }
    }
}

// --- Завантаження ресурсів ---
GLuint loadTexture(const char* path) {
    int w, h, c;
    stbi_set_flip_vertically_on_load(1); 
    unsigned char *data = stbi_load(path, &w, &h, &c, 0);
    if (!data) { printf("Failed to load texture: %s\n", path); return 0; }
    GLuint t;
    glGenTextures(1, &t);
    glBindTexture(GL_TEXTURE_2D, t);
    glTexImage2D(GL_TEXTURE_2D, 0, (c==4)?GL_RGBA:GL_RGB, w, h, 0, (c==4)?GL_RGBA:GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    stbi_image_free(data);
    return t;
}

char* readShader(const char* p) {
    FILE* f = fopen(p, "rb");
    if(!f) return NULL;
    fseek(f, 0, SEEK_END); long l = ftell(f); fseek(f, 0, SEEK_SET);
    char* b = malloc(l + 1); fread(b, 1, l, f); b[l] = 0;
    fclose(f);
    return b;
}

GLuint createProg(const char *vP, const char *fP) {
    char *vS = readShader(vP), *fS = readShader(fP);
    if (!vS || !fS) return 0;
    GLuint v = glCreateShader(GL_VERTEX_SHADER); glShaderSource(v, 1, (const char**)&vS, 0); glCompileShader(v);
    GLuint f = glCreateShader(GL_FRAGMENT_SHADER); glShaderSource(f, 1, (const char**)&fS, 0); glCompileShader(f);
    GLuint p = glCreateProgram(); glAttachShader(p, v); glAttachShader(p, f); glLinkProgram(p);
    free(vS); free(fS);
    return p;
}

void add_static_object(Model* m, float x, float z, float scale) {
    if (staticObjectCount >= MAX_STATIC_OBJECTS) return;
    float y = get_real_terrain_height(x, z);
    staticObjects[staticObjectCount].model = m;
    staticObjects[staticObjectCount].x = x;
    staticObjects[staticObjectCount].y = y;
    staticObjects[staticObjectCount].z = z;
    staticObjects[staticObjectCount].scale = scale;
    staticObjects[staticObjectCount].rotation = 0;

    Box b = m->bounds;
    staticObjects[staticObjectCount].worldBounds = (Box){
        b.minX * scale + x, b.minY * scale + y, b.minZ * scale + z,
        b.maxX * scale + x, b.maxY * scale + y, b.maxZ * scale + z
    };
    staticObjectCount++;
}
void specialKeyDown(int key, int x, int y) {
    // Отримуємо модифікатори ТУТ (це законно в freeglut)
    isShiftPressed = (glutGetModifiers() & GLUT_ACTIVE_SHIFT);

    if (!buildMode) return;

    switch (key) {
        case GLUT_KEY_UP:    buildDistance += 0.5f; break; 
        case GLUT_KEY_DOWN:  buildDistance -= 0.5f; break; 
        case GLUT_KEY_LEFT:  buildRotation -= 5.0f; break; 
        case GLUT_KEY_RIGHT: buildRotation += 5.0f; break; 
    }

    if (buildDistance < 2.0f) buildDistance = 2.0f;
    if (buildDistance > 25.0f) buildDistance = 25.0f;
}

void keyDown(unsigned char k, int x, int y) { 
    // Оновлюємо стан Shift при кожному натисканні
    isShiftPressed = (glutGetModifiers() & GLUT_ACTIVE_SHIFT);

    if (isTyping) {
        if (k == 13) { inputBuffer[inputPos] = 0; isTyping = 0; }
        else if (k == 8 && inputPos > 0) inputPos--;
        else if (inputPos < 63 && k >= 32) inputBuffer[inputPos++] = k;
        return;
    }
    
    keys[k] = 1; 

    if (k == '1') buildMode = !buildMode;
    
    if (buildMode) {
        if (k == '2') buildSelection = &wallModel;
        if (k == '3') buildSelection = &doorModel;
        
        if (k == 'r') {
            buildRotation = floorf((buildRotation + 45.0f) / 90.0f) * 90.0f;
            buildRotation += 90.0f;
            if (buildRotation >= 360.0f) buildRotation -= 360.0f;
            printf("Кут повороту вирівняно: %.0f°\n", buildRotation);
        }

        // Поодинокі натискання для зміни дистанції (клавішами)
        if (k == 'f') buildDistance += 1.0f; 
        if (k == 'v') buildDistance -= 1.0f; 

        if (k == 'p') save_buildings("save.txt"); // P - для збереження (Publish)
        if (k == 'l') load_buildings("save.txt"); // L - для завантаження (Load)

        if (buildDistance < 2.0f)  buildDistance = 2.0f;
        if (buildDistance > 25.0f) buildDistance = 25.0f;
    }

    if(k == 27) exit(0); 
    if(k == 't') { isTyping = 1; inputPos = 0; }

    if (k == 8) { // Backspace
        if (buildingCount > 0) {
            buildingCount--;
            printf("Остання будівля видалена. Залишилось: %d\n", buildingCount);
        }
    }
}

void keyUp(unsigned char k, int x, int y) { 
    // Важливо: скидаємо Shift і тут
    isShiftPressed = (glutGetModifiers() & GLUT_ACTIVE_SHIFT);
    keys[k] = 0; 
}
void updateMouse(int x, int y) {
    if (isTyping) return;
    int cx = glutGet(GLUT_WINDOW_WIDTH) / 2, cy = glutGet(GLUT_WINDOW_HEIGHT) / 2;
    if(x == cx && y == cy) return;
    rotate_camera((float)(x - cx), (float)(y - cy));
    glutWarpPointer(cx, cy);
}

// --- Ініціалізація ---
void init() {
    // 1. Ініціалізація GLEW (має бути першою!)
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        printf("GLEW Error: %s\n", glewGetErrorString(err));
        exit(1);
    }

    // 2. Ініціалізація винесеного рендерера (створює шейдери та буфери неба)
    init_renderer(&rs);

    // 3. Завантаження ігрових ресурсів
    grassTexture = loadTexture("grass.jpg");

    houseModel = load_model("model/house.obj");
    treeModel = load_model("model/tree.obj");
    wallModel = load_model("model/wall_1.obj");
    doorModel = load_model("model/door_arch.obj");
    
    buildSelection = &wallModel;
    
    init_physics(&player);
    init_ui();
    
    // Початкові об'єкти на карті
    add_static_object(&houseModel, 15, 15, 1.5f);
    add_static_object(&treeModel, -10, 5, 0.8f);

    // Стандартні налаштування OpenGL
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS); 
    
    glutSetCursor(GLUT_CURSOR_NONE);
    lastTime = glutGet(GLUT_ELAPSED_TIME);
}

void display() {
    // 1. Розрахунок дельти часу
    int now = glutGet(GLUT_ELAPSED_TIME);
    float dt = (now - lastTime) / 1000.0f;
    lastTime = now;
    if (dt > 0.05f) dt = 0.05f;


    update_world_chunks(cam.x, cam.z);

    dayTime += 0.01f * dt;

    // 2. Оновлення об'єктів для колізій
    objectCount = 0;
    for(int i = 0; i < staticObjectCount; i++) sceneObjects[objectCount++] = staticObjects[i].worldBounds;
    for(int i = 0; i < buildingCount; i++) sceneObjects[objectCount++] = buildings[i].worldBounds;

    // 3. Блок руху
    if (!isTyping) {
        float speedMod = isShiftPressed ? 2.5f : 1.0f;
        float moveStep = playerSpeed * speedMod * dt;
        float fX = cam.frontX, fZ = cam.frontZ;
        
        float len = sqrtf(fX*fX + fZ*fZ);
        if(len > 0) { fX /= len; fZ /= len; }

        float mX = 0, mZ = 0;
        if(keys['w']) { mX += fX; mZ += fZ; }
        if(keys['s']) { mX -= fX; mZ -= fZ; }
        if(keys['a']) { mX += fZ; mZ -= fX; }
        if(keys['d']) { mX -= fZ; mZ += fX; }

        if (physicsEnabled) {
            if(keys[' '] && player.isGrounded) player.verticalVelocity = player.jumpPower;
            float nX = cam.x + mX * moveStep;
            float nZ = cam.z + mZ * moveStep;
            int colX = 0, colZ = 0;
            for(int i = 0; i < objectCount; i++) {
                if(check_collision(nX, cam.y, cam.z, sceneObjects[i])) colX = 1;
                if(check_collision(cam.x, cam.y, nZ, sceneObjects[i])) colZ = 1;
            }
            if(!colX) cam.x = nX;
            if(!colZ) cam.z = nZ;
            update_physics(&player, dt);
        } else {
            cam.x += mX * moveStep * 2; cam.z += mZ * moveStep * 2;
        }
    }

    // 4. Логіка будівництва
    if (buildMode) {
        update_building_logic(keys['b']);
        if (keys['b']) keys['b'] = 0;
    }

// 5. ПЕРЕДАЧА ДАНИХ НА РЕНДЕР
    SceneData sd = {0}; // ОБОВ'ЯЗКОВО {0}, щоб занулити всі нові поля (chunks, numChunks)
    
    sd.dayTime = dayTime;
    sd.chunks = worldChunks;    // Додаємо вказівник на наш масив чанків
    sd.numChunks = NUM_CHUNKS;  // Вказуємо кількість (9)
    
    sd.grassTexture = grassTexture;
    sd.statics = staticObjects;
    sd.staticCount = staticObjectCount;
    sd.buildings = buildings;
    sd.buildCount = buildingCount;
    sd.npcs = npcs;
    sd.npcCount = npcCount;

    // Блок привида
    sd.buildMode = buildMode;
    if (buildMode && buildSelection) {
        sd.buildSelection = buildSelection;
        sd.buildRotation  = buildRotation;
        
        float targetX = cam.x + cam.frontX * buildDistance;
        float targetZ = cam.z + cam.frontZ * buildDistance;
        float gridSize = 4.0f;
        
        sd.buildX = roundf(targetX / gridSize) * gridSize;
        sd.buildZ = roundf(targetZ / gridSize) * gridSize;
        sd.buildY = get_real_terrain_height(sd.buildX, sd.buildZ);
        
        Box previewBox = calculate_rotated_bounds(buildSelection, sd.buildX, sd.buildY, sd.buildZ, buildRotation, 1.5f);
        sd.isColliding = false;
        for (int i = 0; i < buildingCount; i++) {
            if (check_aabb_collision(previewBox, buildings[i].worldBounds)) {
                sd.isColliding = true;
                break;
            }
        }
    }

    // ВИКЛИК РЕНДЕРУ (завжди один раз на кадр в кінці функції)
    render_scene(&rs, &sd);
} // <--- КІНЕЦЬ ФУНКЦІЇ DISPLAY
int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1280, 720);
    glutCreateWindow("Sandbox");
    glutSpecialFunc(specialKeyDown);
    init();
    glutKeyboardFunc(keyDown); glutKeyboardUpFunc(keyUp);
    glutPassiveMotionFunc(updateMouse);
    glutDisplayFunc(display); glutIdleFunc(glutPostRedisplay);
    glutMainLoop();
    return 0;
}