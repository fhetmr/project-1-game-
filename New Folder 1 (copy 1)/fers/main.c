#include <GL/glew.h>
#include <GL/freeglut.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "gen.h"
#include "cam.h"
#include "physik.h" 
#include "npc.h" 


GLuint skyShader, terrainShader;
GLuint skyVAO, skyVBO;
GLuint grassTexture;
TerrainMesh terrain;
NPC myNpc; 

int keys[256] = {0};
float dayTime = 1.5f; 
PlayerPhysics player;

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


void keyDown(unsigned char k, int x, int y) { 
    keys[k] = 1; 
    if(k == 27) exit(0); 
    if(k == 'r' || k == 'R') { 
        myNpc.x = cam.x + 3.0f;
        myNpc.z = cam.z + 3.0f;
        myNpc.physics.verticalVelocity = 0.5f; 
    }
}
void keyUp(unsigned char k, int x, int y) { keys[k] = 0; }

void updateMouse(int x, int y) {
    int w = glutGet(GLUT_WINDOW_WIDTH), h = glutGet(GLUT_WINDOW_HEIGHT);
    if(x == w/2 && y == h/2) return;
    rotate_camera(x, y, w, h);
    glutWarpPointer(w/2, h/2);
}


void init() {
    glewInit();
    skyShader = createProg("sky.vs", "sky.fs");
    terrainShader = createProg("terrain.vs", "terrain.fs");
    grassTexture = loadTexture("grass.jpg");
    terrain = generate_terrain(400, 1.0f);
    
    init_physics(&player);
    

    myNpc = create_npc(cam.x + 5.0f, cam.z + 5.0f);
    

    if(objectCount < 10) {
        sceneObjects[objectCount] = myNpc.bounds;
        objectCount++;
    }

    float skyV[] = { -1,1,0, -1,-1,0, 1,-1,0, -1,1,0, 1,-1,0, 1,1,0 };
    glGenVertexArrays(1, &skyVAO); glGenBuffers(1, &skyVBO);
    glBindVertexArray(skyVAO); glBindBuffer(GL_ARRAY_BUFFER, skyVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyV), skyV, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, 0, 0, 0); glEnableVertexAttribArray(0);

    glEnable(GL_DEPTH_TEST);
    glutSetCursor(GLUT_CURSOR_NONE);
}


void display() {
    dayTime += 0.0003f; 


    update_npc(&myNpc, cam.x, cam.z);
    sceneObjects[0] = myNpc.bounds; 

    float speed = 0.25f;
    float nextX = cam.x, nextZ = cam.z;
    float fwdX = cam.frontX, fwdZ = cam.frontZ;
    float fwdLen = sqrtf(fwdX*fwdX + fwdZ*fwdZ);
    if(fwdLen > 0) { fwdX /= fwdLen; fwdZ /= fwdLen; }
    float rX = fwdZ, rZ = -fwdX;

    if(keys['w']) { nextX += fwdX * speed; nextZ += fwdZ * speed; }
    if(keys['s']) { nextX -= fwdX * speed; nextZ -= fwdZ * speed; }
    if(keys['a']) { nextX += rX * speed; nextZ += rZ * speed; }
    if(keys['d']) { nextX -= rX * speed; nextZ -= rZ * speed; }

    int col = 0;
    for(int i = 0; i < objectCount; i++)
        if(check_collision(nextX, cam.y, nextZ, sceneObjects[i])) { col = 1; break; }

    if(!col) { cam.x = nextX; cam.z = nextZ; }
    if(keys[' '] && player.isGrounded) player.verticalVelocity = player.jumpPower;

    update_physics(&player, 1.0f);


    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    int w = glutGet(GLUT_WINDOW_WIDTH), h = glutGet(GLUT_WINDOW_HEIGHT);
    float view[16], proj[16], identity[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};

    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluPerspective(45.0, (double)w/h, 0.1, 1000.0);
    glGetFloatv(GL_PROJECTION_MATRIX, proj);

  //Sky
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

 //Terrain
    glUseProgram(terrainShader);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    gluLookAt(cam.x, cam.y, cam.z, cam.x+cam.frontX, cam.y+cam.frontY, cam.z+cam.frontZ, 0, 1, 0);
    glGetFloatv(GL_MODELVIEW_MATRIX, view);
    glUniformMatrix4fv(glGetUniformLocation(terrainShader, "model"), 1, 0, identity);
    glUniformMatrix4fv(glGetUniformLocation(terrainShader, "view"), 1, 0, view);
    glUniformMatrix4fv(glGetUniformLocation(terrainShader, "projection"), 1, 0, proj);
    glUniform3f(glGetUniformLocation(terrainShader, "lightPos"), 0, sinf(dayTime)*100, cosf(dayTime)*100);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, grassTexture);
    glBindVertexArray(terrain.vao);
    glDrawElements(GL_TRIANGLES, terrain.indexCount, GL_UNSIGNED_INT, 0);

  //NPC
    glUseProgram(0); 
    glBindVertexArray(0);
    draw_npc(&myNpc, 0);



    glutSwapBuffers();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1280, 720);
    glutCreateWindow("Survival Engine 2026");
    init();
    glutKeyboardFunc(keyDown); glutKeyboardUpFunc(keyUp);
    glutPassiveMotionFunc(updateMouse);
    glutDisplayFunc(display); glutIdleFunc(glutPostRedisplay);
    glutMainLoop();
    return 0;
}