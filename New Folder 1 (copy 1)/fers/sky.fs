#version 330 core
out vec4 FragColor;

uniform vec2 u_res;
uniform vec3 u_sunDir;
uniform mat4 projection;
uniform mat4 view;

void main() {

    vec2 ndc = (gl_FragCoord.xy / u_res) * 2.0 - 1.0;
    

    mat4 invProj = inverse(projection);
    mat4 invView = inverse(view);
    
    vec4 clipPos = vec4(ndc, -1.0, 1.0);
    vec4 viewPos = invProj * clipPos;
    viewPos = vec4(viewPos.xy, -1.0, 0.0);

    vec3 worldDir = normalize((invView * viewPos).xyz);


    float sunDot = max(dot(worldDir, normalize(u_sunDir)), 0.0);
    

    float h = normalize(u_sunDir).y;
    vec3 daySky = vec3(0.3, 0.5, 0.9);
    vec3 nightSky = vec3(0.01, 0.01, 0.02);
    vec3 sunset = vec3(0.8, 0.3, 0.1);

    vec3 skyColor = mix(nightSky, daySky, clamp(h * 4.0, 0.0, 1.0));

    if (h < 0.2 && h > -0.2) {
        skyColor = mix(skyColor, sunset, clamp(1.0 - abs(h * 5.0), 0.0, 1.0));
    }


    float sunDisc = pow(sunDot, 8000.0) * 4.0;
    float glow = pow(sunDot, 400.0) * 0.7;  
    
    vec3 sunColor = vec3(1.0, 0.98, 0.9) * (sunDisc + glow);

    FragColor = vec4(skyColor + sunColor, 1.0);
}