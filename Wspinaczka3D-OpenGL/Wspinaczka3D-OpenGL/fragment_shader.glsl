#version 330 core

out vec4 FragColor;

in vec2 TexCoords;

uniform vec3 objectColor;
uniform sampler2D texture_diffuse1;
uniform int useTexture;

void main()
{
    if (useTexture == 1)
    {
        FragColor = texture(texture_diffuse1, TexCoords);
    }
    else
    {
        FragColor = vec4(objectColor, 1.0);
    }
}
