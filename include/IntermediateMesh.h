#ifndef _INTERMEDIATE_MESH_H_
#define _INTERMEDIATE_MESH_H_

#include <vector>

struct vertexElementFloat
{
  std::vector<float> data;
  unsigned int stride;
};

struct vertexElementInt
{
  std::vector<int> data;
  unsigned int stride;
};

struct vertexWeight
{
  std::vector<float> weights;
  std::vector<int> boneIndices;
};

/** \internal This class is used for temporarily storing mesh data and organizing it so that it can 
             efficiently be packed into Ogre vertex buffers
*/
class CIntermediateMesh
{
public:
  CIntermediateMesh(void);
  virtual ~CIntermediateMesh(void);

  vertexElementFloat m_positions;
  vertexElementFloat m_normals;
  vertexElementFloat m_texCoords;
  vertexElementFloat m_blendWeights;
  vertexElementInt   m_blendIndices;
  vertexElementFloat m_diffuse;

  std::vector<vertexWeight> m_vertexWeights;

  unsigned int m_subMeshCount;
  std::vector<unsigned int> m_vSubMeshVertexCount;

  std::string m_meshName;
  std::vector<std::string> m_vSubMeshNames;
};

#endif //_INTERMEDIATE_MESH_H_
