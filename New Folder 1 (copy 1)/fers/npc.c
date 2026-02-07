#include "npc.h"
#include <math.h>
#include <GL/freeglut.h>

extern float get_real_terrain_height(float x, float z);

NPC create_npc(float x, float z) {
    NPC n;
    n.x = x;
    n.z = z;
    n.angle = 0.0f;
    
    n.physics.height = 1.0f;
    n.physics.verticalVelocity = 0.0f;
    n.physics.gravity = -0.012f;
    n.physics.isGrounded = 0;
    
    n.y = get_real_terrain_height(x, z) + n.physics.height;
    
    float r = 0.75f;
    n.bounds = (Box){x - r, n.y - 1.0f, z - r, x + r, n.y + 1.0f, z + r};
    return n;
}

void update_npc(NPC* n, float playerX, float playerZ) {

    float dx = playerX - n->x;
    float dz = playerZ - n->z;
    n->angle = atan2f(dx, dz) * 180.0f / 3.14159f;


    n->physics.verticalVelocity += n->physics.gravity;
    n->y += n->physics.verticalVelocity;

    float groundY = get_real_terrain_height(n->x, n->z);
    

    if (n->y < groundY + n->physics.height) {
        n->y = groundY + n->physics.height;
        n->physics.verticalVelocity = 0;
        n->physics.isGrounded = 1;
    } else {
        n->physics.isGrounded = 0;
    }


    float r = 0.75f;
    n->bounds = (Box){n->x - r, n->y - 1.0f, n->z - r, 
                      n->x + r, n->y + 1.0f, n->z + r};
}

void draw_npc(NPC* n, GLuint shader) {
    glPushMatrix();
    glTranslatef(n->x, n->y, n->z);
    glRotatef(n->angle, 0, 1, 0);

    float s = 0.75f;

    glBegin(GL_QUADS);

        glColor3f(1.0f, 1.0f, 1.0f);
        glVertex3f(-s, -s,  s); glVertex3f( s, -s,  s);
        glVertex3f( s,  s,  s); glVertex3f(-s,  s,  s);


        glVertex3f(-s, -s, -s); glVertex3f(-s,  s, -s);
        glVertex3f( s,  s, -s); glVertex3f( s, -s, -s);


        glColor3f(0.9f, 0.9f, 0.9f);
        glVertex3f(-s,  s, -s); glVertex3f(-s,  s,  s);
        glVertex3f( s,  s,  s); glVertex3f( s,  s, -s);


        glVertex3f(-s, -s, -s); glVertex3f( s, -s, -s);
        glVertex3f( s, -s,  s); glVertex3f(-s, -s,  s);


        glColor3f(0.8f, 0.8f, 0.8f);
        glVertex3f( s, -s, -s); glVertex3f( s,  s, -s);
        glVertex3f( s,  s,  s); glVertex3f( s, -s,  s);


        glVertex3f(-s, -s, -s); glVertex3f(-s, -s,  s);
        glVertex3f(-s,  s,  s); glVertex3f(-s,  s, -s);
    glEnd();


    glColor3f(0.0f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
        glVertex3f(-0.2f, -0.2f, s + 0.1f);
        glVertex3f( 0.2f, -0.2f, s + 0.1f);
        glVertex3f( 0.2f,  0.2f, s + 0.1f);
        glVertex3f(-0.2f,  0.2f, s + 0.1f);
    glEnd();

    glPopMatrix();
}