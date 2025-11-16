#pragma once
#ifndef MODEL_H
#define MODEL_H

#include <string>
#include <vector>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

#include "Mesh.h"
#include "Shader.h"
class Shader;

unsigned int TextureFromFile(const char* path, const std::string& directory);

class Model
{
public:
    Model(std::string const& path);
    void Draw(Shader& shader);

private:
    std::vector<Mesh> meshes;
    std::string directory;

    void loadModel(std::string const& path);
    void processNode(aiNode* node, const aiScene* scene);
    Mesh processMesh(aiMesh* mesh, const aiScene* scene);
    std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName);
};

#endif

