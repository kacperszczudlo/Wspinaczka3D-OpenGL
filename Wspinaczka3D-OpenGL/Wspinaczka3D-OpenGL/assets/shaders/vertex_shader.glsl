#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out VS_OUT {
    vec3 FragPos;        // pozycja w œwiecie
    vec3 Normal;         // normal w œwiecie
    vec2 TexCoords;      // UV
    vec4 FragPosLightSpace; // pod cienie
} vs_out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Dla cieni (na razie mo¿esz ustawiæ na macierz jednostkow¹, ale docelowo podasz z C++)
uniform mat4 lightSpaceMatrix;

void main()
{
    vec4 worldPos = model * vec4(aPos, 1.0);

    vs_out.FragPos = worldPos.xyz;

    // normal matrix poprawia normalne przy skalowaniu w model matrix
    mat3 normalMatrix = transpose(inverse(mat3(model)));
    vs_out.Normal = normalize(normalMatrix * aNormal);

    vs_out.TexCoords = aTexCoords;

    vs_out.FragPosLightSpace = lightSpaceMatrix * worldPos;

    gl_Position = projection * view * worldPos;
}
