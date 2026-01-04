#version 330 core

out vec4 FragColor;

in vec2 TexCoords;

// ZMIANA 1: vec4 zamiast vec3, aby przyjmowaæ przezroczystoœæ (Alpha)
uniform vec4 objectColor;

// ZMIANA 2: int zamiast bool, aby pasowa³o do shader.setInt("useTexture", 1) w C++
uniform int useTexture;

uniform sampler2D texture_diffuse1; // Nazwa zgodna z Twoim poprzednim plikiem

void main()
{
    // Domyœlny kolor to bia³y (neutralny przy mno¿eniu)
    vec4 texColor = vec4(1.0);

    // Jeœli u¿ywamy tekstury, pobieramy jej kolor
    if (useTexture == 1)
    {
        texColor = texture(texture_diffuse1, TexCoords);
    }

    // ZMIANA 3: Mieszamy (mno¿ymy) kolor tekstury z kolorem obiektu.
    // Dziêki temu, jeœli objectColor.a (Alpha) jest np. 0.5, ca³y obiekt bêdzie pó³przezroczysty.
    FragColor = texColor * objectColor;
}