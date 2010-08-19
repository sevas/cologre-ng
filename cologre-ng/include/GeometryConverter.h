#ifndef _GEOMETRY_CONVERTER_H_
#define _GEOMETRY_CONVERTER_H_

#include "ResourceConverter.h"
#include "IntermediateMesh.h"
#include "dom/domGeometry.h"

struct IntermediateVertexData
{
  unsigned int vertexCount;
  unsigned int vertexSize;
  std::vector<float> data;
};

///Class for converting collada geometry into ogre meshes.
/**Class for converting collada geometry into ogre meshes. It creates a Ogre::Mesh of every collada "<mesh>" definition and one submesh
  for each collada "<triangle>" definition within that "<mesh>".
*/
class CGeometryConverter : public CResourceConverter
{
public:
    ///Contructor
    CGeometryConverter(Ogre::Log *_log);
    ///Destructor
    virtual ~CGeometryConverter(void);

    /**Reads collada "<geometry>s" and creates Ogre::Meshes accordingly. It creates vertex buffers and copies all vertex, normal and texture
      data to the buffer. Created meshes are stored in Ogres global MeshManager. 
      An Ogre root object has to have been created BEFORE calling this function.
      @param pDatabase daeDatabse to convert the geometry elements from
      @return 0 if succeeded, 1 upon error, prints error message to stderr
    */
    int convert(daeDatabase* pDatabase);

protected:
    ///parses the input array of a "<triangle>" tag to create an Ogre vertex declaration
    void buildVertexDeclFromInputArray(const daeElementRef elemRef, Ogre::MeshPtr pOgreMesh);
    /**reads the "<p>" values of a "<triangle>" section, reindexes all values and then copies all vertex, normal and texture coordinate values
      to the right location in the vertex buffer, according tpo their new index, fills index buffer with new indices.
    */
    void copyData(domPRef pRef, const std::vector<domSource*> &vSource, IntermediateVertexData* pDest, Ogre::VertexDeclaration* pVertexDecl, Ogre::AxisAlignedBox* pAABB);

    ///for temporarily storing position, normal, txcoord, etc values 
    std::vector<float> m_vDataTmp;
    ///for temporarily storing offsets in the m_vDataTmp vector 
    std::vector<int> m_vDataTmpOffsets;
    ///vertexCount for a single submesh
    unsigned int m_vertexCount;
    ///the dominant vertex element (the one with the highest count). Needed to create enough entries in the ogre vertex buffer.
    unsigned int m_dominantElementIndex;
    ///keep atrack of all vertex declarations
    std::vector<Ogre::VertexDeclaration*> m_vDeclarations;
    std::vector<IntermediateVertexData> m_vIVD;

    ///temporary storage place for vertex-bone-assignments
    std::vector<vertexWeight> m_vVBAssignments;

    conversion_errors loadGeometryToIntermediateMesh(domGeometry* pGeo, CIntermediateMesh* pIM);
    void addVertexWeights(domSkin* pSkin);
    void makeOgreMeshFromIntermediateMesh(CIntermediateMesh* pIM);

    Ogre::Log *m_pLog;

};

#endif //_GEOMETRY_COONVERTER_H_