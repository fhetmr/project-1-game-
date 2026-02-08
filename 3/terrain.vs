#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;
out float WindStrength;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform float u_time;

uniform vec2  u_windDir;
uniform float u_windPower;
uniform float u_windGust;
uniform vec3  viewPos;

//
// ---------------- NOISE ----------------
//
float hash(vec2 p){
    p = fract(p * vec2(234.34, 435.21));
    p += dot(p, p + 34.23);
    return fract(p.x * p.y);
}

float noise(vec2 p){
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f*f*(3.0-2.0*f);

    float a = hash(i);
    float b = hash(i+vec2(1,0));
    float c = hash(i+vec2(0,1));
    float d = hash(i+vec2(1,1));

    return mix(mix(a,b,f.x), mix(c,d,f.x), f.y);
}

void main()
{
    vec3 pos = aPos;

    vec2 windDir = normalize(u_windDir);
    float t = u_time;

    // ---------------- базові хвилі ----------------
    float big   = noise(pos.xz * 0.03 + t * 0.15);
    float mid   = noise(pos.xz * 0.09 - t * 0.35);
    float small = sin(pos.x * 0.5 + t * 2.0) * 0.2;

    float baseWave = big * 0.7 + mid * 0.5 + small;

    // ---------------- gusts ----------------
    float gustField = noise(pos.xz * 0.01 + t * 0.05);
    float gust = mix(0.4, 1.8, gustField) * u_windGust;

    float wave = baseWave * gust;

    // ---------------- LOD ----------------
    vec3 worldPos = vec3(model * vec4(pos, 1.0));
    float dist = distance(worldPos, viewPos);
    float lod = 1.0 - smoothstep(60.0, 250.0, dist);
    wave *= lod;

    // ---------------- сила ----------------
    wave *= u_windPower;

    WindStrength = wave;

    // ---------------- рух ----------------
    pos.xz += windDir * wave * 0.25;
    pos.y  += wave * 0.05;

    // ---------------- трансформації ----------------
    vec4 world = model * vec4(pos, 1.0);

    FragPos = world.xyz;
    Normal  = normalize(mat3(transpose(inverse(model))) * aNormal);
    TexCoord = aTexCoord + windDir * wave * 0.01;

    gl_Position = projection * view * world;
}
