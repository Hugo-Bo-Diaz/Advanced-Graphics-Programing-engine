#ifndef ASSIMP_MODEL_LOADING
#define ASSIMP_MODEL_LOADING

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "engine.h"

void ProcessAssimpMesh(const aiScene* scene, aiMesh *mesh, Mesh *myMesh, u32 baseMeshMaterialIndex, std::vector<u32>& submeshMaterialIndices);




#endif