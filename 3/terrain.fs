#version 330 core

out vec4 FragColor;

in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;

uniform sampler2D grassTex;
uniform vec3 lightPos;
uniform vec3 viewPos;

// координати і розмір чанку для туману по краях
uniform vec2 chunkCenterXZ;
uniform float chunkHalfSize;

// ---------- NOISE
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

// ---------- ACES
vec3 ACESFilm(vec3 x){
    return clamp((x*(2.51*x+0.03))/(x*(2.43*x+0.59)+0.14),0.0,1.0);
}

void main()
{
    // 1. ТЕКСТУРА + ЗЕЛЕНИЙ ТОН
    vec3 tex = texture(grassTex, TexCoord).rgb;
    tex.r *= 0.8;    
    tex.g *= 1.15;   
    tex.b *= 0.95;
    vec3 deepGreen = vec3(0.03, 0.28, 0.07);
    tex = mix(tex, deepGreen, 0.35);
    float n = noise(FragPos.xz * 0.015);
    tex *= mix(0.85, 1.15, n);

    // 2. СВІТЛО
    vec3 N = normalize(Normal);
    vec3 L = normalize(lightPos - FragPos);
    vec3 V = normalize(viewPos - FragPos);
    float sunHeight = clamp(lightPos.y / 600.0, 0.0, 1.0);
    vec3 sunColor = mix(vec3(1.0,0.8,0.6), vec3(0.9,1.0,1.05), sunHeight);
    float diff = pow(max(dot(N, L), 0.0), 1.4);
    vec3 diffuse = diff * sunColor;

    // 3. НЕБО + АМБІЄНТ
    vec3 sky = mix(vec3(0.02,0.03,0.06),
                   vec3(0.45,0.6,0.8),
                   sunHeight);
    float hemi = 0.5 + 0.5 * N.y;
    vec3 ambient = sky * hemi * 0.8;

    // 4. М’ЯКЕ ПРОСВІЧУВАННЯ ТРАВИ
    float sss = pow(max(dot(V, -L),0.0), 8.0);
    vec3 sssCol = vec3(0.1,0.45,0.1) * 0.3 * sss;

    // 5. КОМПОЗИЦІЯ
    vec3 result = tex * (diffuse + ambient) + sssCol;



    // 7. ТОН + ГАММА
    result = ACESFilm(result * 1.05);
    result = pow(result, vec3(1.0/2.2));

    FragColor = vec4(result, 1.0);
}
