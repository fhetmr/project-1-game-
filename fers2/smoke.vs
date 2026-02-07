#version 330 core
layout (location = 0) in vec3 aPos;
uniform mat4 view;
uniform mat4 projection;
uniform float u_time;

out float vAlpha;

void main() {

    vec3 pos = aPos;
    pos.y += mod(u_time * 0.5 + aPos.x, 10.0); 
    
    gl_Position = projection * view * vec4(pos, 1.0);
    

    gl_PointSize = 1000.0 / gl_Position.w; 
    vAlpha = 1.0 - (mod(u_time * 0.5 + aPos.x, 10.0) / 10.0);
}