#include "assets.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

auto gengine::load_vertex_buffer(std::string_view path) -> std::tuple<std::vector<float>, std::vector<unsigned int>>
{
    Assimp::Importer importer;
    
    const auto scene = importer.ReadFile(path.data(), aiProcess_Triangulate | aiProcess_GenNormals);

    //Check for errors
    if ((!scene) || (scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE) || (!scene->mRootNode))
    {
        return {};
    }

    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    //Iterate over the meshes
    for (auto i = 0; i < scene->mNumMeshes; ++i)
    {
        //Get the mesh
        const auto mesh = scene->mMeshes[i];

        //Iterate over the vertices of the mesh
        for (auto j = 0; j < mesh->mNumVertices; ++j)
        {
            vertices.push_back(mesh->mVertices[j].x);
            vertices.push_back(-mesh->mVertices[j].y);
            vertices.push_back(mesh->mVertices[j].z);

            vertices.push_back(mesh->mNormals[j].x);
            vertices.push_back(mesh->mNormals[j].y);
            vertices.push_back(mesh->mNormals[j].z);

            vertices.push_back(mesh->mTextureCoords[0][j].x);
            vertices.push_back(mesh->mTextureCoords[0][j].y);
        }

        //Iterate over the faces of the mesh
        for (auto j = 0; j < mesh->mNumFaces; ++j)
        {
            //Get the face
            const auto face = mesh->mFaces[j];

            //Add the indices of the face to the vector
            for (auto k = 0; k < face.mNumIndices; ++k)
            {
                indices.push_back(face.mIndices[k]);
            }
        }
    }

    return { vertices, indices };
}