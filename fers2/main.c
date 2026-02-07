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

extern float get_real_terrain_height(float x, float z);

// --- Глобальні змінні стану світу ---
GLuint skyShader, terrainShader;
GLuint skyVAO, skyVBO;
GLuint grassTexture;
TerrainMesh terrain;

Model treeModel;


// --- Система сутностей (Entities) ---
#define MAX_NPCS 50
NPC npcs[MAX_NPCS];
int npcCount = 0;

// --- Стан гравця та введення ---
int keys[256] = {0};
float dayTime = 1.5f; 
PlayerPhysics player;
int isTyping = 0;
char inputBuffer[64];
int inputPos = 0;

// --- Параметри розробника (Команди) ---
int physicsEnabled = 1;     
float playerSpeed = 0.25f;  

// --- Допоміжні функції рендерингу ---
GLuint loadTexture(const char* path) {
    int w, h, c;
    stbi_set_flip_vertically_on_load(1); 
    unsigned char *data = stbi_load(path, &w, &h, &c, 0);
    if (!data) return 0;
    GLuint t;
    glGenTextures(1, &t);
    glBindTexture(GL_TEXTURE_2D, t);
    glTexImage2D(GL_TEXTURE_2D, 0, (c==4)?GL_RGBA:GL_RGB, w, h, 0, (c==4)?GL_RGBA:GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);
    return t;
}

char* readShader(const char* p) {
    FILE* f = fopen(p, "rb");
    if(!f) return NULL;
    fseek(f, 0, SEEK_END); long l = ftell(f); fseek(f, 0, SEEK_SET);
    char* b = malloc(l + 1); fread(b, 1, l, f); b[l] = 0; fclose(f);
    return b;
}

GLuint createProg(const char *vP, const char *fP) {
    const char *vS = readShader(vP), *fS = readShader(fP);
    GLuint v = glCreateShader(GL_VERTEX_SHADER); glShaderSource(v, 1, &vS, 0); glCompileShader(v);
    GLuint f = glCreateShader(GL_FRAGMENT_SHADER); glShaderSource(f, 1, &fS, 0); glCompileShader(f);
    GLuint p = glCreateProgram(); glAttachShader(p, v); glAttachShader(p, f); glLinkProgram(p);
    return p;
}

// --- Логіка команд чату ---
void process_command(const char* cmd) {
    if (strcmp(cmd, "/nophys") == 0) {
        physicsEnabled = !physicsEnabled;
        add_chat_message(physicsEnabled ? "System: Physics Enabled" : "System: NoClip Enabled");
    } 
    else if (strncmp(cmd, "/speed ", 7) == 0) {
        float s = (float)atof(cmd + 7);
        if (s > 0) {
            playerSpeed = s;
            char msg[64]; sprintf(msg, "System: Speed set to %.2f", s);
            add_chat_message(msg);
        }
    }
    else if (strcmp(cmd, "/spawn") == 0) {
        if (npcCount < MAX_NPCS) {
            float rx = cam.x + (rand() % 16) - 8;
            float rz = cam.z + (rand() % 16) - 8;
            npcs[npcCount] = create_npc(rx, rz);
            npcCount++;
            add_chat_message("System: New NPC spawned nearby.");
        } else {
            add_chat_message("System: Error - NPC limit reached!");
        }
    }
    else if (strcmp(cmd, "/clear") == 0) {
        npcCount = 0;
        add_chat_message("System: All NPCs removed.");
    }
    else if (strcmp(cmd, "/day") == 0) {
        dayTime = 1.5f;
        add_chat_message("System: Set time to Midday.");
    }
    else if (strcmp(cmd, "/night") == 0) {
        dayTime = 4.7f;
        add_chat_message("System: Set time to Midnight.");
    }
    else {
        add_chat_message(cmd); // Просто вивід тексту, якщо не команда
    }
}

// --- Обробка вводу ---
void keyDown(unsigned char k, int x, int y) { 
    if (isTyping) {
        if (k == 13) { // ENTER
            inputBuffer[inputPos] = '\0';
            process_command(inputBuffer);
            isTyping = 0; inputPos = 0;
        } else if (k == 27) { // ESC
            isTyping = 0;
        } else if (k == 8 && inputPos > 0) { // Backspace
            inputPos--;
        } else if (inputPos < 63 && k >= 32) {
            inputBuffer[inputPos++] = k;
        }
        return;
    }

    keys[k] = 1; 
    if(k == 27) exit(0); 
    if(k == 't' || k == 'T') { 
        isTyping = 1; 
        inputPos = 0; 
        inputBuffer[0] = '\0'; 
    }
}

void keyUp(unsigned char k, int x, int y) { keys[k] = 0; }

void updateMouse(int x, int y) {
    if (isTyping) return;
    int w = glutGet(GLUT_WINDOW_WIDTH), h = glutGet(GLUT_WINDOW_HEIGHT);
    if(x == w/2 && y == h/2) return;
    rotate_camera(x, y, w, h);
    glutWarpPointer(w/2, h/2);
}

// --- Ініціалізація ---
void init() {
    srand((unsigned int)time(NULL));
    glewInit();
    
    skyShader = createProg("sky.vs", "sky.fs");
    terrainShader = createProg("terrain.vs", "terrain.fs");
    grassTexture = loadTexture("grass.jpg");
    terrain = generate_terrain(400, 1.0f);
    
    treeModel = load_model("model/house.obj");

    init_physics(&player);
    init_ui();

    // Початковий NPC
    npcs[0] = create_npc(cam.x + 5.0f, cam.z + 5.0f);
    npcCount = 1;

    // Геометрія неба (Full-screen quad)
    float skyV[] = { -1,1,0, -1,-1,0, 1,-1,0, -1,1,0, 1,-1,0, 1,1,0 };
    glGenVertexArrays(1, &skyVAO); glGenBuffers(1, &skyVBO);
    glBindVertexArray(skyVAO); glBindBuffer(GL_ARRAY_BUFFER, skyVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyV), skyV, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, 0, 0, 0); glEnableVertexAttribArray(0);

    glEnable(GL_DEPTH_TEST);
    glutSetCursor(GLUT_CURSOR_NONE);
    add_chat_message("Engine initialized. Type /spawn to create NPCs.");
}

// --- Основний цикл рендерингу та логіки ---
void display() {
    dayTime += 0.0002f; // Повільна зміна доби
    
    // 1. Оновлення штучного інтелекту та колізій NPC
    objectCount = 0;
    for (int i = 0; i < npcCount; i++) {
        // Передаємо контекст для колізій між ботами
        update_npc(&npcs[i], cam.x, cam.z, npcs, npcCount, i);
        // Заповнюємо масив об'єктів для фізики гравця
        if (objectCount < 100) {
            sceneObjects[objectCount++] = npcs[i].bounds;
        }
    }

    // 2. Логіка руху гравця
    if (!isTyping) {
        float nextX = cam.x, nextZ = cam.z;
        float fwdX = cam.frontX, fwdZ = cam.frontZ;
        float fwdLen = sqrtf(fwdX*fwdX + fwdZ*fwdZ);
        if(fwdLen > 0) { fwdX /= fwdLen; fwdZ /= fwdLen; }
        float rX = fwdZ, rZ = -fwdX;

        if(keys['w']) { nextX += fwdX * playerSpeed; nextZ += fwdZ * playerSpeed; }
        if(keys['s']) { nextX -= fwdX * playerSpeed; nextZ -= fwdZ * playerSpeed; }
        if(keys['a']) { nextX += rX * playerSpeed; nextZ += rZ * playerSpeed; }
        if(keys['d']) { nextX -= rX * playerSpeed; nextZ -= rZ * playerSpeed; }

        if (physicsEnabled) {
            int col = 0;
            for(int i = 0; i < objectCount; i++)
                if(check_collision(nextX, cam.y, nextZ, sceneObjects[i])) { col = 1; break; }
            if(!col) { cam.x = nextX; cam.z = nextZ; }
            if(keys[' '] && player.isGrounded) player.verticalVelocity = player.jumpPower;
            update_physics(&player, 1.0f);
        } else {
            // Режим польоту (NoClip)
            cam.x = nextX; cam.z = nextZ;
            if(keys[' ']) cam.y += playerSpeed;
            if(keys['c'] || keys['C']) cam.y -= playerSpeed;
        }
    }

    // 3. Візуалізація сцени
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    int w = glutGet(GLUT_WINDOW_WIDTH), h = glutGet(GLUT_WINDOW_HEIGHT);
    float view[16], proj[16], identity[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};

    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluPerspective(45.0, (double)w/h, 0.1, 1000.0);
    glGetFloatv(GL_PROJECTION_MATRIX, proj);

    // --- Малюємо Небо ---
    glDisable(GL_DEPTH_TEST);
    glUseProgram(skyShader);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    gluLookAt(0, 0, 0, cam.frontX, cam.frontY, cam.frontZ, 0, 1, 0);
    glGetFloatv(GL_MODELVIEW_MATRIX, view);
    glUniformMatrix4fv(glGetUniformLocation(skyShader, "view"), 1, 0, view);
    glUniformMatrix4fv(glGetUniformLocation(skyShader, "projection"), 1, 0, proj);
    glUniform3f(glGetUniformLocation(skyShader, "u_sunDir"), 0, sinf(dayTime), cosf(dayTime));
    glUniform2f(glGetUniformLocation(skyShader, "u_res"), (float)w, (float)h);
    glBindVertexArray(skyVAO); glDrawArrays(GL_TRIANGLES, 0, 6);
    glEnable(GL_DEPTH_TEST);

    // --- Малюємо Ландшафт ---
    glUseProgram(terrainShader);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    gluLookAt(cam.x, cam.y, cam.z, cam.x+cam.frontX, cam.y+cam.frontY, cam.z+cam.frontZ, 0, 1, 0);
    glGetFloatv(GL_MODELVIEW_MATRIX, view);
    glUniformMatrix4fv(glGetUniformLocation(terrainShader, "model"), 1, 0, identity);
    glUniformMatrix4fv(glGetUniformLocation(terrainShader, "view"), 1, 0, view);
    glUniformMatrix4fv(glGetUniformLocation(terrainShader, "projection"), 1, 0, proj);
    glUniform3f(glGetUniformLocation(terrainShader, "lightPos"), 0, sinf(dayTime)*100, cosf(dayTime)*100);
    glBindTexture(GL_TEXTURE_2D, grassTexture);
    glBindVertexArray(terrain.vao);
    glDrawElements(GL_TRIANGLES, terrain.indexCount, GL_UNSIGNED_INT, 0);


// Малюємо дерево
    glColor3f(0.2f, 0.5f, 0.1f);
    draw_model(&treeModel, 10.0f, get_real_terrain_height(10, 10), 10.0f, 1.0f);

    // Додаємо колізію дерева до списку об'єктів (враховуючи зміщення)
    Box treeHitbox = treeModel.bounds;
    treeHitbox.minX += 10.0f; treeHitbox.maxX += 10.0f;
    treeHitbox.minZ += 10.0f; treeHitbox.maxZ += 10.0f;
    sceneObjects[objectCount++] = treeHitbox;

    // --- Малюємо Всіх NPC ---
    glUseProgram(0); 
    for (int i = 0; i < npcCount; i++) {
        draw_npc(&npcs[i], 0);
    }

    // --- Інтерфейс (UI) ---
    // Міні-карта тепер відображає всіх NPC
   draw_ui(w, h, cam.x, cam.z, 0.0f, npcs, npcCount, &terrain);
    
    // Рендеринг активного рядка вводу
    if (isTyping) {
        glColor3f(1, 1, 0); // Жовтий колір для вводу
        char displayBuf[128]; sprintf(displayBuf, "> %s|", inputBuffer);
        glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity(); gluOrtho2D(0, w, 0, h);
        glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
        glRasterPos2f(20, 100);
        for(int i=0; displayBuf[i]; i++) glutBitmapCharacter(GLUT_BITMAP_9_BY_15, displayBuf[i]);
        glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW);
    }

    glutSwapBuffers();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1280, 720);
    glutCreateWindow("Survival Sandbox 2026");
    init();
    glutKeyboardFunc(keyDown); glutKeyboardUpFunc(keyUp);
    glutPassiveMotionFunc(updateMouse);
    glutDisplayFunc(display); glutIdleFunc(glutPostRedisplay);
    glutMainLoop();
    return 0;
}