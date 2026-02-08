#include "ui.h"
#include <GL/freeglut.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#define MAX_CHAT_MESSAGES 5
#define MAX_MSG_LEN 64

typedef struct {
    char messages[MAX_CHAT_MESSAGES][MAX_MSG_LEN];
    int count;
} Chat;

Chat gameChat;

void init_ui() {
    gameChat.count = 0;
    for(int i = 0; i < MAX_CHAT_MESSAGES; i++) {
        gameChat.messages[i][0] = '\0';
    }
}

void add_chat_message(const char* text) {
    if (gameChat.count < MAX_CHAT_MESSAGES) {
        strncpy(gameChat.messages[gameChat.count], text, MAX_MSG_LEN);
        gameChat.count++;
    } else {
        for (int i = 1; i < MAX_CHAT_MESSAGES; i++) {
            strcpy(gameChat.messages[i-1], gameChat.messages[i]);
        }
        strncpy(gameChat.messages[MAX_CHAT_MESSAGES-1], text, MAX_MSG_LEN);
    }
}

void draw_text(float x, float y, const char* text) {
    glRasterPos2f(x, y);
    for (const char* c = text; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    }
}

void draw_ui(int w, int h, float px, float pz, float pa, NPC* npcs, int npcCount, TerrainMesh* terrain) {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float mapSize = 180.0f;
    float margin = 20.0f;
    float mapX = margin;
    float mapY = margin + 110.0f;
    float viewDist = 45.0f; 

    // 1. Вмикаємо "ножиці" (Scissor Test)
    // Важливо: Scissor використовує координати вікна (знизу-вгору)
    glEnable(GL_SCISSOR_TEST);
    glScissor((int)mapX, (int)mapY, (int)mapSize, (int)mapSize);

    // 2. ФОН КАРТИ
    glColor4f(0.0f, 0.05f, 0.1f, 0.9f);
    glBegin(GL_QUADS);
        glVertex2f(mapX, mapY);
        glVertex2f(mapX + mapSize, mapY);
        glVertex2f(mapX + mapSize, mapY + mapSize);
        glVertex2f(mapX, mapY + mapSize);
    glEnd();

    // 3. ОБ'ЄКТИ (NPC)
    for(int i = 0; i < npcCount; i++) {
        float rnx = ((npcs[i].x - px) / viewDist) * (mapSize/2.0f);
        float rnz = ((npcs[i].z - pz) / viewDist) * (mapSize/2.0f);
        if(fabs(rnx) < mapSize/2 && fabs(rnz) < mapSize/2) {
            float sx = mapX + mapSize/2 + rnx;
            float sy = mapY + mapSize/2 + rnz;
            glPointSize(5.0f);
            glColor4f(1.0f, 0.9f, 0.0f, 1.0f);
            glBegin(GL_POINTS); glVertex2f(sx, sy); glEnd();
        }
    }

    // 4. ОБ'ЄМНИЙ ЛАНДШАФТ (Плитки)
    float step = 3.0f; 
    for(float ox = -viewDist; ox < viewDist; ox += step) {
        for(float oz = -viewDist; oz < viewDist; oz += step) {
            extern float get_real_terrain_height(float x, float z);
            
            float h1 = get_real_terrain_height(px + ox,        pz + oz);
            float h2 = get_real_terrain_height(px + ox + step, pz + oz);
            float h3 = get_real_terrain_height(px + ox + step, pz + oz + step);
            float h4 = get_real_terrain_height(px + ox,        pz + oz + step);

            float x1 = mapX + mapSize/2 + (ox / viewDist) * (mapSize/2);
            float y1 = mapY + mapSize/2 + (oz / viewDist) * (mapSize/2);
            float x2 = mapX + mapSize/2 + ((ox + step) / viewDist) * (mapSize/2);
            float y2 = mapY + mapSize/2 + ((oz + step) / viewDist) * (mapSize/2);

            glBegin(GL_QUADS);
                for(int i = 0; i < 4; i++) {
                    float val = (i==0)?h1 : (i==1)?h2 : (i==2)?h3 : h4;
                    float c = (val + 10.0f) / 25.0f; 
                    if(c < 0.1f) c = 0.1f;
                    glColor4f(c * 0.1f, c * 0.5f, c * 0.3f, 0.6f);
                    
                    float vx = (i==0||i==3)?x1 : x2;
                    float vy = (i==0||i==1)?y1 : y2;
                    glVertex2f(vx, vy + val * 2.0f); 
                }
            glEnd();
        }
    }

    // 5. ГРАВЕЦЬ
    glPointSize(7.0f);
    glColor4f(1.0f, 0.2f, 0.2f, 1.0f);
    glBegin(GL_POINTS);
        glVertex2f(mapX + mapSize/2, mapY + mapSize/2);
    glEnd();

    // Вимикаємо Scissor, щоб він не обрізав чат і рамку
    glDisable(GL_SCISSOR_TEST);



    // 7. ЧАТ
    glColor3f(1, 1, 1);
    for (int i = 0; i < gameChat.count; i++) {
        draw_text(margin, margin + (i * 18), gameChat.messages[i]);
    }

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW); glPopMatrix();
}