#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 vWorldDir; // Напрямок для фрагментного шейдера

uniform mat4 view;
uniform mat4 projection;

void main() {
    // Встановлюємо позицію вершини квадрата
    gl_Position = vec4(aPos, 1.0);

    // Видова матриця без трансляції (небо закріплене)
    mat4 viewRot = mat4(mat3(view));

    // Перетворення із кліп-простору у view-space
    vec4 clipPos = vec4(aPos.xy, -1.0, 1.0);
    vec4 viewPos = inverse(projection) * clipPos;
    viewPos /= viewPos.w;

    // Напрямок у world-space
    vWorldDir = normalize((inverse(viewRot) * vec4(viewPos.xyz, 0.0)).xyz);
}
