/*
 * Copyright (C) 2022 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include "gz/common/graphics/Types.hh"
#include "gz/common/Console.hh"
#include "gz/common/Material.hh"
#include "gz/common/SubMesh.hh"
#include "gz/common/Mesh.hh"
#include "gz/common/Skeleton.hh"
#include "gz/common/SkeletonAnimation.hh"
#include "gz/common/SystemPaths.hh"
#include "gz/common/Util.hh"
#include "gz/common/AssimpLoader.hh"

#include <unordered_set>

#include <assimp/Logger.hpp>      // C++ importer interface
#include <assimp/DefaultLogger.hpp>      // C++ importer interface
#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags

namespace ignition
{
namespace common
{

/// \brief Private data for the AssimpLoader class
class AssimpLoader::Implementation
{
  /// Convert a color from assimp implementation to Ignition common
  public: ignition::math::Color ConvertColor(aiColor4D& _color);

  /// Convert a transformation from assimp implementation to Ignition math
  public: ignition::math::Matrix4d ConvertTransform(aiMatrix4x4& _matrix);

  public: MaterialPtr CreateMaterial(const aiScene* scene, unsigned mat_idx, const std::string& path);
};

ignition::math::Color AssimpLoader::Implementation::ConvertColor(aiColor4D& _color)
{
  ignition::math::Color col(_color.r, _color.g, _color.b, _color.a);
  return col;
}

ignition::math::Matrix4d AssimpLoader::Implementation::ConvertTransform(aiMatrix4x4& _sm)
{
  return ignition::math::Matrix4d(
      _sm.a1, _sm.a2, _sm.a3, _sm.a4,
      _sm.b1, _sm.b2, _sm.b3, _sm.b4,
      _sm.c1, _sm.c2, _sm.c3, _sm.c4,
      _sm.d1, _sm.d2, _sm.d3, _sm.d4);
}

//////////////////////////////////////////////////
MaterialPtr AssimpLoader::Implementation::CreateMaterial(const aiScene* scene, unsigned mat_idx, const std::string& path)
{
  MaterialPtr mat = std::make_shared<Material>();
  aiColor4D color;
  igndbg << "Processing material with name " << scene->mMaterials[mat_idx]->GetName().C_Str() << std::endl;
  auto ret = scene->mMaterials[mat_idx]->Get(AI_MATKEY_COLOR_DIFFUSE, color);
  if (ret == AI_SUCCESS)
  {
    mat->SetDiffuse(this->ConvertColor(color));
  }
  ret = scene->mMaterials[mat_idx]->Get(AI_MATKEY_COLOR_AMBIENT, color);
  if (ret == AI_SUCCESS)
  {
    mat->SetAmbient(this->ConvertColor(color));
  }
  ret = scene->mMaterials[mat_idx]->Get(AI_MATKEY_COLOR_SPECULAR, color);
  if (ret == AI_SUCCESS)
  {
    mat->SetSpecular(this->ConvertColor(color));
  }
  ret = scene->mMaterials[mat_idx]->Get(AI_MATKEY_COLOR_EMISSIVE, color);
  if (ret == AI_SUCCESS)
  {
    mat->SetEmissive(this->ConvertColor(color));
  }
  double shininess;
  ret = scene->mMaterials[mat_idx]->Get(AI_MATKEY_SHININESS, shininess);
  if (ret == AI_SUCCESS)
  {
    mat->SetShininess(shininess);
  }
  // TODO more than one texture
  aiString texturePath(path.c_str());
  unsigned textureIndex = 0;
  unsigned uvIndex = 10000;
  ret = scene->mMaterials[mat_idx]->GetTexture(aiTextureType_DIFFUSE, textureIndex, &texturePath,
      NULL, // Type of mapping, TODO check that it is UV
      &uvIndex,
      NULL, // Blend mode, TODO implement
      NULL, // Texture operation, unneeded?
      NULL); // Mapping modes, unneeded?
  if (ret == AI_SUCCESS)
  {
    mat->SetTextureImage(std::string(texturePath.C_Str()), path.c_str());
    if (uvIndex < 10000)
    {

    }
  }
  // TODO other properties
  return mat;
}

//////////////////////////////////////////////////
AssimpLoader::AssimpLoader()
: MeshLoader(), dataPtr(ignition::utils::MakeImpl<Implementation>())
{
}

//////////////////////////////////////////////////
AssimpLoader::~AssimpLoader()
{
}

//////////////////////////////////////////////////
Mesh *AssimpLoader::Load(const std::string &_filename)
{
  // TODO share importer
  Mesh *mesh = new Mesh();
  std::string path = common::parentPath(_filename);
  Assimp::DefaultLogger::create("", Assimp::Logger::VERBOSE, aiDefaultLogStream_STDOUT);
  Assimp::Importer importer;
  importer.SetPropertyBool(AI_CONFIG_PP_FD_REMOVE, true);
  // Load the asset, TODO check if we need to do preprocessing
  const aiScene* scene = importer.ReadFile(_filename,
      //aiProcess_JoinIdenticalVertices |
      aiProcess_RemoveRedundantMaterials |
      aiProcess_SortByPType |
      aiProcess_Triangulate |
      0);
  if (scene == nullptr)
  {
    ignerr << "Unable to import mesh [" << _filename << "]" << std::endl;
    return mesh;
  }
  auto root_node = scene->mRootNode;
  auto root_transformation = this->dataPtr->ConvertTransform(scene->mRootNode->mTransformation);
  // TODO remove workaround, it seems imported assets are rotated by 90 degrees
  // as documented here https://github.com/assimp/assimp/issues/849, remove workaround when fixed
  // TODO find actual workaround to remove rotation
  // root_transformation = root_transformation * root_transformation.Rotation();
  // TODO recursive call for children?
  // Add the materials first
  for (unsigned mat_idx = 0; mat_idx < scene->mNumMaterials; ++mat_idx)
  {
    auto mat = this->dataPtr->CreateMaterial(scene, mat_idx, path);
    mesh->AddMaterial(mat);
  }
  for (unsigned node_idx = 0; node_idx < root_node->mNumChildren; ++node_idx)
  {
    auto node = root_node->mChildren[node_idx];
    // TODO node name
    ignmsg << "Processing mesh with " << node->mNumMeshes << " meshes" << std::endl;
    for (unsigned mesh_idx = 0; mesh_idx < node->mNumMeshes; ++mesh_idx)
    {
      SubMesh subMesh;
      auto assimp_mesh_idx = node->mMeshes[mesh_idx];
      auto assimp_mesh = scene->mMeshes[assimp_mesh_idx];
      auto trans = this->dataPtr->ConvertTransform(node->mTransformation);
      trans = root_transformation * trans;
      ignition::math::Matrix4d rot = trans;
      rot.SetTranslation(ignition::math::Vector3d::Zero);
      // Now create the submesh
      for (unsigned vertex_idx = 0; vertex_idx < assimp_mesh->mNumVertices; ++vertex_idx)
      {
        // Add the vertex
        ignition::math::Vector3d vertex;
        ignition::math::Vector3d normal;
        vertex.X(assimp_mesh->mVertices[vertex_idx].x);
        vertex.Y(assimp_mesh->mVertices[vertex_idx].y);
        vertex.Z(assimp_mesh->mVertices[vertex_idx].z);
        normal.X(assimp_mesh->mNormals[vertex_idx].x);
        normal.Y(assimp_mesh->mNormals[vertex_idx].y);
        normal.Z(assimp_mesh->mNormals[vertex_idx].z);
        vertex = trans * vertex;
        normal = rot * normal;
        normal.Normalize();
        subMesh.AddVertex(vertex);
        subMesh.AddNormal(normal);
        // Iterate over sets of texture coordinates
        int i = 0;
        while(assimp_mesh->HasTextureCoords(i))
        {
          ignition::math::Vector3d texcoords;
          texcoords.X(assimp_mesh->mTextureCoords[i][vertex_idx].x);
          texcoords.Y(assimp_mesh->mTextureCoords[i][vertex_idx].y);
          // TODO why do we need 1.0 - Y?
          subMesh.AddTexCoordBySet(texcoords.X(), 1.0 - texcoords.Y(), i);
          ++i;
        }
      }
      for (unsigned face_idx = 0; face_idx < assimp_mesh->mNumFaces; ++face_idx)
      {
        auto face = assimp_mesh->mFaces[face_idx];
        subMesh.AddIndex(face.mIndices[0]);
        subMesh.AddIndex(face.mIndices[1]);
        subMesh.AddIndex(face.mIndices[2]);
      }
      subMesh.SetName(std::string(node->mName.C_Str()));

      ignmsg << "Submesh " << node->mName.C_Str() << " has material index " << assimp_mesh->mMaterialIndex << std::endl;
      subMesh.SetMaterialIndex(assimp_mesh->mMaterialIndex);
      mesh->AddSubMesh(std::move(subMesh));
    }
  }
  // Process animations
  ignmsg << "Processing " << scene->mNumAnimations << " animations" << std::endl;
  ignmsg << "Scene has " << scene->mNumMeshes << " meshes" << std::endl;
  // Iterate over meshes in scene not contained in root node
  // this is a strict superset of the above that also contains animation meshes
  for (unsigned mesh_idx = 0; mesh_idx < scene->mNumMeshes; ++mesh_idx)
  {
    // Skip if the mesh was found in the previous step
    auto mesh_name = std::string(scene->mMeshes[mesh_idx]->mName.C_Str());
    if (!mesh->SubMeshByName(mesh_name).expired())
      continue;
    auto assimp_mesh = scene->mMeshes[mesh_idx];
    ignmsg << "New mesh found with name " << scene->mMeshes[mesh_idx]->mName.C_Str() << std::endl;
  }
  for (unsigned anim_idx = 0; anim_idx < scene->mNumAnimations; ++anim_idx)
  {
    auto anim = scene->mAnimations[anim_idx];
    ignmsg << "Animation has " << anim->mNumMeshChannels << " mesh channels" << std::endl;
    ignmsg << "Animation has " << anim->mNumChannels << " channels" << std::endl;
    ignmsg << "Animation has " << anim->mNumMorphMeshChannels << " morph mesh channels" << std::endl;
    // Process t
  }
  // Iterate over nodes and add a submesh for each
  /*
  mesh->SetPath(this->dataPtr->path);

  this->dataPtr->LoadScene(mesh);

  if (mesh->HasSkeleton())
    this->dataPtr->ApplyInvBindTransform(mesh->MeshSkeleton());

  // This will make the model the correct size.
  mesh->Scale(ignition::math::Vector3d(
      this->dataPtr->meter, this->dataPtr->meter, this->dataPtr->meter));
  if (mesh->HasSkeleton())
    mesh->MeshSkeleton()->Scale(this->dataPtr->meter);
  */

  return mesh;
}

}
}
