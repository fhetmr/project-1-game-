#version 130
uniform sampler2D diffuseMap; 
uniform sampler2D specularMap; 
varying vec3 Normal;
varying vec3 FragPos;
varying vec2 TexCoord;

void main() {
    vec3 lightPos = vec3(5.0, 5.0, 5.0);
    vec3 lightColor = vec3(1.0, 1.0, 1.0);

    // 1. Ambient 
    vec3 ambient = 0.2 * texture2D(diffuseMap, TexCoord).rgb;

    // 2. Diffuse 
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * texture2D(diffuseMap, TexCoord).rgb;

    // 3. Specular 
    float roughness = texture2D(specularMap, TexCoord).r; 
    vec3 viewDir = normalize(-FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = spec * roughness * lightColor; 

    gl_FragColor = vec4(ambient + diffuse + specular, 1.0);
}