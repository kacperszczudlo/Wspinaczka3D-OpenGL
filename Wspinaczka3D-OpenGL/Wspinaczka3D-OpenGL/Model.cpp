#include "Model.h"
#include "stb_image.h"

#include <glad/glad.h>
#include <iostream>
#include "Shader.h"

// =====================
// Konstruktor modelu
// =====================
Model::Model(std::string const& path)
{
    loadModel(path);
}

void Model::Draw(Shader& shader)
{
    for (auto& mesh : meshes)
    {
        mesh.Draw(shader);
    }
}

// =====================
// Wczytywanie modelu
// =====================

void Model::loadModel(std::string const& path)
{
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(
        path,
        aiProcess_Triangulate |
        aiProcess_FlipUVs |
        aiProcess_GenSmoothNormals
    );

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
        return;
    }

    directory = path.substr(0, path.find_last_of("/\\"));

    processNode(scene->mRootNode, scene);
}

void Model::processNode(aiNode* node, const aiScene* scene)
{
    // wszystkie meshe przypisane do tego noda
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(processMesh(mesh, scene));
    }

    // rekurencyjnie przetwarzamy dzieci
    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        processNode(node->mChildren[i], scene);
    }
}

Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene)
{
    std::vector<Vertex>   vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture>  textures;

    // ----- vertexy -----
    vertices.reserve(mesh->mNumVertices);
    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        Vertex vertex;

        // pozycja
        vertex.Position = glm::vec3(
            mesh->mVertices[i].x,
            mesh->mVertices[i].y,
            mesh->mVertices[i].z
        );

        // normalne
        if (mesh->HasNormals())
        {
            vertex.Normal = glm::vec3(
                mesh->mNormals[i].x,
                mesh->mNormals[i].y,
                mesh->mNormals[i].z
            );
        }
        else
        {
            vertex.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
        }

        // wspó³rzêdne tekstury (UV) – kana³ 0
        if (mesh->mTextureCoords[0])
        {
            vertex.TexCoords = glm::vec2(
                mesh->mTextureCoords[0][i].x,
                mesh->mTextureCoords[0][i].y
            );
        }
        else
        {
            vertex.TexCoords = glm::vec2(0.0f, 0.0f);
        }

        vertices.push_back(vertex);
    }

    // ----- indeksy -----
    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
        {
            indices.push_back(face.mIndices[j]);
        }
    }

    // ----- tekstury -----
    if (mesh->mMaterialIndex >= 0)
    {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        // diffuse (kolor)
        std::vector<Texture> diffuseMaps =
            loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

        // mo¿esz dodaæ specular/normal póŸniej, na razie nie s¹ wymagane
    }

    return Mesh(vertices, indices, textures);
}

std::vector<Texture> Model::loadMaterialTextures(aiMaterial* mat,
    aiTextureType type,
    std::string typeName)
{
    std::vector<Texture> textures;

    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
    {
        aiString str;
        mat->GetTexture(type, i, &str);

        Texture texture;
        texture.id = TextureFromFile(str.C_Str(), directory);
        texture.type = typeName;
        texture.path = str.C_Str();

        textures.push_back(texture);
    }

    return textures;
}

// =====================
// £adowanie tekstury z pliku
// =====================

unsigned int TextureFromFile(const char* path, const std::string& directory)
{
    std::string filename = path;
    // jeœli œcie¿ka w .mtl jest wzglêdna, doklejamy katalog
    if (!directory.empty())
    {
        filename = directory + "/" + filename;
    }

    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format = GL_RGB;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0,
            format, width, height,
            0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Failed to load texture at path: " << filename << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}
