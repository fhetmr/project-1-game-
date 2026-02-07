#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;

uniform sampler2D grassTex;
uniform vec3 lightPos;
uniform float sunHeight; 

void main() {
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);


    float diff = max(dot(norm, lightDir), 0.0);
    vec3 sunColor = vec3(1.0, 0.9, 0.8); 
    vec3 diffuse = diff * sunColor;



    vec3 ambientNight = vec3(0.05, 0.05, 0.1); 
    vec3 ambientDay = vec3(0.2, 0.25, 0.4);  
    

    vec3 ambient = mix(ambientNight, ambientDay, clamp(lightPos.y / 500.0, 0.0, 1.0));


    vec3 texColor = texture(grassTex, TexCoord).rgb;
    

    vec3 result = (diffuse + ambient) * texColor;

    FragColor = vec4(result, 1.0);
}