#version 330 core

out vec4 FragColor;
in vec3 vWorldDir;

uniform vec3 u_sunDir;
uniform float u_time;
uniform sampler2D cloudTexture;

// ------------------------------------------------
// FBM / шум
// ------------------------------------------------
float fbm(vec2 uv)
{
    float v = 0.0;
    float a = 0.5;
    for(int i=0;i<5;i++)
    {
        v += texture(cloudTexture, uv).r * a;
        uv *= 2.0;
        a *= 0.5;
    }
    return v;
}

// ------------------------------------------------
// одна банка хмар з вертикальним масштабуванням
// ------------------------------------------------
float cloudBank(vec2 uv, float scale, vec2 wind, float power, vec2 offset, float verticalScale)
{
    uv += offset;
    scale *= verticalScale; // зменшуємо/збільшуємо по висоті
    float d = fbm(uv * scale + wind);
    d = smoothstep(0.55, 0.8, d);
    return d * power;
}

// ------------------------------------------------
// усі шари хмар
// ------------------------------------------------
vec3 renderClouds(vec3 dir)
{
    float horizonFactor = clamp(dir.y + 0.05, 0.05, 1.0);
    vec2 uv = dir.xz * 0.35 / horizonFactor;

    float t = u_time;
    float d = 0.0;

    // вертикальний масштаб: центер = 1.0, зверху менші 0.6, низ 0.5
    float verticalScaleTop    = mix(0.6, 1.0, clamp(dir.y,0.0,1.0)); // зверху трохи менші
    float verticalScaleBottom = mix(0.5, 1.0, clamp(dir.y,0.0,1.0)); // низ у 2 рази менший

    // три великі скупчення з вертикальним масштабом
// головний шар: великі пухкі банки
d += cloudBank(uv, 0.15, vec2(t*0.001, t*0.0005), 1.0, vec2(0.0,0.0), verticalScaleTop); // масштаб ще менший → банка більше
d += cloudBank(uv, 0.15, vec2(t*0.001, t*0.0005), 1.0, vec2(1.5,0.5), verticalScaleTop);
d += cloudBank(uv, 0.15, vec2(t*0.001, t*0.0005), 1.0, vec2(-1.2,-0.7), verticalScaleBottom);


    // легкий шар для м’яких контурів
    d += cloudBank(uv, 0.6, vec2(t*0.003, t*0.002), 0.3, vec2(0.0,0.0), 1.0);

    d = clamp(d, 0.0, 1.0);

    // освітлення хмар
    float sunDot = max(dot(normalize(dir), normalize(u_sunDir)), 0.0);
    float silver = pow(sunDot, 6.0);
    float shade  = mix(0.6, 1.0, sunDot);

    vec3 col = vec3(1.0) * shade + vec3(1.0,0.95,0.85) * silver * 1.5;

    // edge mask → плавне згасання на горизонті
    float edgeMask = smoothstep(0.0, 0.05, dir.y + 0.05);
    col *= edgeMask * d;

    return col;
}

// ------------------------------------------------
// main
// ------------------------------------------------
void main()
{
    vec3 topSky  = vec3(0.18,0.45,0.9);
    vec3 horizon = vec3(0.75,0.9,1.0);
    float t = pow(clamp(vWorldDir.y*0.5+0.5,0.0,1.0),0.6);
    vec3 skyColor = mix(horizon, topSky, t);

    float sunDot = max(dot(normalize(u_sunDir), normalize(vWorldDir)), 0.0);
    vec3 sunColor = vec3(1.0,0.97,0.9) * pow(sunDot, 1500.0);

    vec3 cloudCol = renderClouds(normalize(vWorldDir));

    vec3 finalColor = skyColor + cloudCol + sunColor;

    FragColor = vec4(finalColor, 1.0);
}
