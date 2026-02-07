#ifndef UI_H
#define UI_H
#include "gen.h"
#include "npc.h" 

void init_ui(); 
void add_chat_message(const char* text);
// ui.h
void draw_ui(int w, int h, float px, float pz, float pa, NPC* npcs, int npcCount, TerrainMesh* terrain);

#endif