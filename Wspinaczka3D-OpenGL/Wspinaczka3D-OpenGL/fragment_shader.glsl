#version 330 core
out vec4 FragColor;

// Uniform, który ustawimy w kodzie C++ dla ka¿dego obiektu osobno
uniform vec3 objectColor;

void main()
{
    FragColor = vec4(objectColor, 1.0f);
}