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



// --- Зовнішні дані (з build_manager.c та інших) ---
extern int buildMode;
extern Model* buildSelection;
extern float buildRotation;
extern StaticObject buildings[500]; // Має збігатися з MAX_BUILDINGS
extern int buildingCount;
extern float buildDistance;        // ДОДАЙ ЦЕЙ РЯДОК

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
    glewInit();
    skyShader = createProg("sky.vs", "sky.fs");
    terrainShader = createProg("terrain.vs", "terrain.fs");

glUseProgram(terrainShader);

windDirLoc   = glGetUniformLocation(terrainShader, "u_windDir");
windPowerLoc = glGetUniformLocation(terrainShader, "u_windPower");
windGustLoc  = glGetUniformLocation(terrainShader, "u_windGust");
viewPosLoc   = glGetUniformLocation(terrainShader, "viewPos");

GLuint timeLoc;
timeLoc = glGetUniformLocation(terrainShader, "u_time");

    grassTexture = loadTexture("grass.jpg");
    terrain = generate_terrain(400, 1.0f);
    ;

    houseModel = load_model("model/house.obj");
    treeModel = load_model("model/tree.obj");
    wallModel = load_model("model/wall_1.obj");
    doorModel = load_model("model/door_arch.obj");
    
    buildSelection = &wallModel;
    
    init_physics(&player);
    init_ui();
    
    add_static_object(&houseModel, 15, 15, 1.5f);
    add_static_object(&treeModel, -10, 5, 0.8f);

    float skyV[] = { -1,1,0, -1,-1,0, 1,-1,0, -1,1,0, 1,-1,0, 1,1,0 };
    glGenVertexArrays(1, &skyVAO); glGenBuffers(1, &skyVBO);
    glBindVertexArray(skyVAO); glBindBuffer(GL_ARRAY_BUFFER, skyVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyV), skyV, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, 0, 0, 0); glEnableVertexAttribArray(0);

    glEnable(GL_DEPTH_TEST);
    glutSetCursor(GLUT_CURSOR_NONE);
    lastTime = glutGet(GLUT_ELAPSED_TIME);
}

// --- Основний цикл ---
void display() {
    int now = glutGet(GLUT_ELAPSED_TIME);
    float dt = (now - lastTime) / 1000.0f;
    lastTime = now;
    if (dt > 0.05f) dt = 0.05f;

    dayTime += 0.01f * dt;

    // --- Положення сонця ---
    float sunX = 0.0f;
    float sunY = sinf(dayTime);
    float sunZ = cosf(dayTime);

    // --- Збирання колізійних об'єктів ---
    objectCount = 0;
    for(int i = 0; i < staticObjectCount; i++) sceneObjects[objectCount++] = staticObjects[i].worldBounds;
    for(int i = 0; i < buildingCount; i++) sceneObjects[objectCount++] = buildings[i].worldBounds;

    // --- Рух гравця ---
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
            float nX = cam.x + mX * moveStep, nZ = cam.z + mZ * moveStep;
            int colX = 0, colZ = 0;
            for(int i=0; i<objectCount; i++) {
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

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    int w = glutGet(GLUT_WINDOW_WIDTH), h = glutGet(GLUT_WINDOW_HEIGHT);
    float view[16], proj[16], identity[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};

    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluPerspective(60.0, (double)w/(h?h:1), 0.1, 1000.0);
    glGetFloatv(GL_PROJECTION_MATRIX, proj);

// --- Небо ---
glDisable(GL_DEPTH_TEST);
glUseProgram(skyShader);

// тільки оберт камери (skybox логіка)
glMatrixMode(GL_MODELVIEW);
glLoadIdentity();
gluLookAt(0, 0, 0,
          cam.frontX, cam.frontY, cam.frontZ,
          0, 1, 0);

glGetFloatv(GL_MODELVIEW_MATRIX, view);

// прибираємо трансляцію
float viewRot[16];
memcpy(viewRot, view, sizeof(float) * 16);
viewRot[12] = 0.0f;
viewRot[13] = 0.0f;
viewRot[14] = 0.0f;

// матриці
glUniformMatrix4fv(glGetUniformLocation(skyShader, "view"), 1, GL_FALSE, viewRot);
glUniformMatrix4fv(glGetUniformLocation(skyShader, "projection"), 1, GL_FALSE, proj);

// напрям сонця
glUniform3f(glGetUniformLocation(skyShader, "u_sunDir"), sunX, sunY, sunZ);

// позиція камери (КРИТИЧНО для volumetric clouds)
glUniform3f(glGetUniformLocation(skyShader, "u_cameraPos"),
             cam.x, cam.y, cam.z);

// роздільна здатність
glUniform2f(glGetUniformLocation(skyShader, "u_res"),
            (float)w, (float)h);

// час
float time = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
glUniform1f(glGetUniformLocation(skyShader, "u_time"), time);

// малюємо небо
glBindVertexArray(skyVAO);
glDrawArrays(GL_TRIANGLES, 0, 6);

glEnable(GL_DEPTH_TEST);


    // --- Ландшафт ---
    glUseProgram(terrainShader);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    gluLookAt(cam.x, cam.y, cam.z, cam.x+cam.frontX, cam.y+cam.frontY, cam.z+cam.frontZ, 0,1,0);
    glGetFloatv(GL_MODELVIEW_MATRIX, view);

    glUniformMatrix4fv(glGetUniformLocation(terrainShader, "model"), 1, 0, identity);
    glUniformMatrix4fv(glGetUniformLocation(terrainShader, "view"), 1, 0, view);
    glUniformMatrix4fv(glGetUniformLocation(terrainShader, "projection"), 1, 0, proj);

    glUniform3f(glGetUniformLocation(terrainShader, "u_sunDir"), sunX, sunY, sunZ);
    glUniform2f(windDirLoc, 1.0f, 0.3f);
    glUniform1f(windPowerLoc, 1.0f);
    glUniform1f(windGustLoc, 0.7f);

    glBindTexture(GL_TEXTURE_2D, grassTexture);
    glBindVertexArray(terrain.vao);
    glDrawElements(GL_TRIANGLES, terrain.indexCount, GL_UNSIGNED_INT, 0);
    glUseProgram(0);

    // --- Статичні моделі ---
    for(int i=0; i<staticObjectCount; i++)
        draw_model(staticObjects[i].model, staticObjects[i].x, staticObjects[i].y, staticObjects[i].z, staticObjects[i].scale);

    // --- Будівлі ---
    for(int i=0; i<buildingCount; i++) {
        glPushMatrix();
            glTranslatef(buildings[i].x, buildings[i].y, buildings[i].z);
            glRotatef(buildings[i].rotation, 0, 1, 0);
            glScalef(buildings[i].scale, buildings[i].scale, buildings[i].scale);

            float ox = (buildings[i].model->bounds.minX + buildings[i].model->bounds.maxX) / 2.0f;
            float oz = (buildings[i].model->bounds.minZ + buildings[i].model->bounds.maxZ) / 2.0f;
            glTranslatef(-ox, 0, -oz);

            glBindVertexArray(buildings[i].model->vao);
            glDrawArrays(GL_TRIANGLES, 0, buildings[i].model->vertexCount);
            glBindVertexArray(0);
        glPopMatrix();
    }

    // --- Будівництво (привид) ---
    if (buildMode) {
        if(keys['q']) buildRotation -= 2.0f;
        if(keys['e']) buildRotation += 2.0f;
        if(keys['f']) buildDistance += 0.2f;
        if(keys['v']) buildDistance -= 0.2f;

        if (buildDistance < 2.0f) buildDistance = 2.0f;
        if (buildDistance > 25.0f) buildDistance = 25.0f;

        update_building_logic(keys['b']);
        if (keys['b']) keys['b'] = 0;
    }

    draw_ui(w, h, cam.x, cam.z, 0.0f, npcs, npcCount, &terrain);
    glutSwapBuffers();
}


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