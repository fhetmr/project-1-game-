#version 330 core
out vec4 FragColor;
in float vAlpha;

void main() {

    vec2 circ = gl_PointCoord - vec2(0.5);
    float dist = length(circ);
    if (dist > 0.5) discard;
    
    float mask = smoothstep(0.5, 0.0, dist);
    FragColor = vec4(0.8, 0.8, 0.8, mask * vAlpha * 0.4);
}