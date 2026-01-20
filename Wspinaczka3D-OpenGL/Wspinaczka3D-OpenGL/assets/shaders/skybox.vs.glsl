#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 TexCoords;

uniform mat4 view;
uniform mat4 projection;

void main()
{
    TexCoords = aPos;
    mat4 viewNoTranslate = mat4(mat3(view)); // usuñ translacjê
    vec4 pos = projection * viewNoTranslate * vec4(aPos, 1.0);
    gl_Position = pos.xyww; // depth = 1.0
}
