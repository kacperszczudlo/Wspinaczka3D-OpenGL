#version 330 core
out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
    vec4 FragPosLightSpace;
} fs_in;

// Twoje istniej¹ce:
uniform vec4 objectColor;
uniform int useTexture;
uniform sampler2D texture_diffuse1;
uniform int forceUpNormal;
uniform int twoSided;



// Nowe:
uniform vec3 viewPos;

// directional light (najprostsze i dobre do cieni)
uniform vec3 lightDir;     // kierunek œwiat³a (np. (-0.3, -1.0, -0.2))
uniform vec3 lightColor;   // np. (1,1,1)

// Cienie:
uniform sampler2D shadowMap;

// PCF + bias
float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDirNorm)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    // poza map¹
    if (projCoords.z > 1.0) return 0.0;

    float currentDepth = projCoords.z;

    // bias (wa¿ne przeciw "shadow acne")
    float bias = max(0.002 * (1.0 - dot(normal, -lightDirNorm)), 0.0005);

    // PCF 3x3
    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0));
    for (int x = -1; x <= 1; ++x)
    for (int y = -1; y <= 1; ++y)
    {
        float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
        shadow += (currentDepth - bias > pcfDepth) ? 1.0 : 0.0;
    }
    shadow /= 9.0;

    return shadow;
}

void main()
{
    vec4 texColor = vec4(1.0);
    if (useTexture == 1)
        texColor = texture(texture_diffuse1, fs_in.TexCoords);

    vec4 baseColor = texColor * objectColor;

    // Jeœli masz "cutout" (dziury) — odetnij piksele prawie przezroczyste
    // (dla np. roœlin/siatek). Jeœli nie u¿ywasz, mo¿esz to usun¹æ.
    if (baseColor.a < 0.05)
        discard;


    //vec3 norm = normalize(fs_in.Normal);

    //vec3 norm = fs_in.Normal;
    //if (length(norm) < 0.0001) norm = vec3(0.0, 1.0, 0.0);
    //else norm = normalize(norm);

    //if (forceUpNormal == 1) norm = vec3(0.0, 1.0, 0.0);
    //else if (!gl_FrontFacing) norm = -norm;

    vec3 norm = fs_in.Normal;
    if (length(norm) < 0.0001) norm = vec3(0.0, 1.0, 0.0);
    else norm = normalize(norm);

    // tylko jeœli obiekt tego potrzebuje
    if (twoSided == 1 && !gl_FrontFacing)
        norm = -norm;

    // tylko jeœli wymuszamy
    if (forceUpNormal == 1)
        norm = vec3(0.0, 1.0, 0.0);


    

    vec3 L = normalize(-lightDir); // bo lightDir trzymamy jako "kierunek padania"
    vec3 V = normalize(viewPos - fs_in.FragPos);

    // Ambient
    vec3 ambient = 0.35 * lightColor;

    // Diffuse
    float diff = max(dot(norm, L), 0.0);
    vec3 diffuse = diff * lightColor;

    // Specular
    vec3 R = reflect(-L, norm);
    float spec = pow(max(dot(V, R), 0.0), 32.0);
    vec3 specular = 0.5 * spec * lightColor;

    float shadow = ShadowCalculation(fs_in.FragPosLightSpace, norm, L);

    // ambient bez cienia, reszta przyciemniana cieniem
    vec3 lighting = ambient + (1.0 - shadow) * (diffuse + specular);

    vec3 rgb = lighting * baseColor.rgb;
    FragColor = vec4(rgb, baseColor.a);
}
